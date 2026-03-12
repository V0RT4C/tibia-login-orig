#include "login/login_handler.h"

#include <iterator>

#include "common.h"
#include "crypto/rsa.h"
#include "query/query_client.h"
#include "query/login_query.h"
#include "query/query_types.h"
#include "config/config.h"
#include "crypto/xtea.h"

#if TIBIA772
static const int TERMINALVERSION[] = {772, 772, 772};
#else
static const int TERMINALVERSION[] = {770, 770, 770};
#endif

// Login Request
//==============================================================================
static BufferWriter prepare_xtea_response(TConnection *connection){
	if(connection->State != CONNECTION_PROCESSING){
		LOG_ERR("Connection %s is not PROCESSING (State: %d)",
				connection->RemoteAddress, connection->State);
		CloseConnection(connection);
		return BufferWriter(NULL, 0);
	}

	BufferWriter write_buffer(connection->Buffer, sizeof(connection->Buffer));
	write_buffer.write_u16(0); // Encrypted Size
	write_buffer.write_u16(0); // Data Size
	return write_buffer;
}

static void send_xtea_response(TConnection *connection, BufferWriter *write_buffer){
	if(connection->State != CONNECTION_PROCESSING){
		LOG_ERR("Connection %s is not PROCESSING (State: %d)",
				connection->RemoteAddress, connection->State);
		CloseConnection(connection);
		return;
	}

	ASSERT(write_buffer != NULL
		&& write_buffer->buffer == connection->Buffer
		&& write_buffer->size == sizeof(connection->Buffer)
		&& write_buffer->position > 4);

	int data_size = write_buffer->position - 4;
	int encrypted_size = write_buffer->position - 2;
	while((encrypted_size % 8) != 0){
		write_buffer->write_u8(rand_r(&connection->RandomSeed));
		encrypted_size += 1;
	}

	if(write_buffer->overflowed()){
		LOG_ERR("Write buffer overflowed when writing response to %s",
				connection->RemoteAddress);
		CloseConnection(connection);
		return;
	}

	write_buffer->rewrite_u16(0, encrypted_size);
	write_buffer->rewrite_u16(2, data_size);
	xtea_encrypt(connection->XTEA,
			write_buffer->buffer + 2,
			write_buffer->position - 2);
	connection->State = CONNECTION_WRITING;
	connection->RWSize = write_buffer->position;
	connection->RWPosition = 0;
}

static void send_login_error(TConnection *connection, const char *message){
	BufferWriter write_buffer = prepare_xtea_response(connection);
	write_buffer.write_u8(10); // LOGIN_ERROR
	write_buffer.write_string(message);
	send_xtea_response(connection, &write_buffer);
}

static void send_character_list(TConnection *connection, int num_characters,
		CharacterLoginData *characters, int premium_days, const ServerConfig& config){
	BufferWriter write_buffer = prepare_xtea_response(connection);

	if(config.motd[0] != 0){
		write_buffer.write_u8(20); // MOTD
		write_buffer.write_string(config.motd);
	}

	write_buffer.write_u8(100); // CHARACTER_LIST
	if(num_characters > UINT8_MAX){
		num_characters = UINT8_MAX;
	}
	write_buffer.write_u8(num_characters);
	for(int i = 0; i < num_characters; i += 1){
		write_buffer.write_string(characters[i].name);
		write_buffer.write_string(characters[i].world_name);
		write_buffer.write_u32_be((uint32)characters[i].world_address);
		write_buffer.write_u16((uint16)characters[i].world_port);
	}
	write_buffer.write_u16((uint16)premium_days);

	send_xtea_response(connection, &write_buffer);
}

void process_login_request(TConnection *connection, RsaKey& rsa_key,
		QueryClient& client, const ServerConfig& config){
	if(connection->RWSize != 145){
		LOG_ERR("Invalid login request size from %s (expected 145, got %d)",
				connection->RemoteAddress, connection->RWSize);
		CloseConnection(connection);
		return;
	}

	BufferReader read_buffer(connection->Buffer, connection->RWSize);
	read_buffer.read_u8(); // always 1 for a login request
	read_buffer.read_u16(); // TerminalType (OS)
	int terminal_version = read_buffer.read_u16();
	read_buffer.read_u32(); // DATSIGNATURE
	read_buffer.read_u32(); // SPRSIGNATURE
	read_buffer.read_u32(); // PICSIGNATURE

	uint8 asymmetric_data[128];
	read_buffer.read_bytes(asymmetric_data, sizeof(asymmetric_data));
	if(read_buffer.overflowed()){
		LOG_ERR("Input buffer overflowed while reading login command from %s",
				connection->RemoteAddress);
		CloseConnection(connection);
		return;
	}

	// IMPORTANT(fusion): Without a checksum, there is no way of validating
	// the asymmetric data. The best we can do is to verify that the first
	// plaintext byte is ZERO, but that alone isn't enough.
	if(!rsa_key.decrypt(asymmetric_data, sizeof(asymmetric_data)) || asymmetric_data[0] != 0){
		LOG_ERR("Failed to decrypt asymmetric data from %s",
				connection->RemoteAddress);
		CloseConnection(connection);
		return;
	}

	read_buffer = BufferReader(asymmetric_data, sizeof(asymmetric_data));
	read_buffer.read_u8(); // always zero
	connection->XTEA[0] = read_buffer.read_u32();
	connection->XTEA[1] = read_buffer.read_u32();
	connection->XTEA[2] = read_buffer.read_u32();
	connection->XTEA[3] = read_buffer.read_u32();

	char password[30];
	int account_id = read_buffer.read_u32();
	read_buffer.read_string(password, sizeof(password));
	if(read_buffer.overflowed()){
		LOG_ERR("Malformed asymmetric data from %s", connection->RemoteAddress);
		CloseConnection(connection);
		return;
	}

	if(account_id <= 0){
		send_login_error(connection, "You must enter an account number.");
		return;
	}

	if(terminal_version != TERMINALVERSION[0]){
		send_login_error(connection,
				"Your terminal version is too old.\n"
				"Please get a new version at\n"
				"http://www.tibia.com.");
		return;
	}

	char ip_string[16];
	string_buf_format(ip_string, "%d.%d.%d.%d",
			((connection->IPAddress >> 24) & 0xFF),
			((connection->IPAddress >> 16) & 0xFF),
			((connection->IPAddress >>  8) & 0xFF),
			((connection->IPAddress >>  0) & 0xFF));

	int num_characters = 0;
	int premium_days = 0;
	CharacterLoginData characters[50];
	int login_code = login_account(client, account_id, password, ip_string,
			static_cast<int>(std::size(characters)), &num_characters, characters, &premium_days);
	switch(login_code){
		case 0:{
			send_character_list(connection, num_characters, characters, premium_days, config);
			break;
		}

		case 1:		// Invalid account number
		case 2:{	// Invalid password
			send_login_error(connection, "Accountnumber or password is not correct.");
			break;
		}

		case 3:{
			send_login_error(connection, "Account disabled for five minutes. Please wait.");
			break;
		}

		case 4:{
			send_login_error(connection, "IP address blocked for 30 minutes. Please wait.");
			break;
		}

		case 5:{
			send_login_error(connection, "Your account is banished.");
			break;
		}

		case 6:{
			send_login_error(connection, "Your IP address is banished.");
			break;
		}

		default:{
			if(login_code != -1){
				LOG_ERR("Invalid login code %d", login_code);
			}
			send_login_error(connection, "Internal error, closing connection.");
			break;
		}
	}
}
