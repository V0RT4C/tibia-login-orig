#include "login/login_handler.h"

#include <iterator>
#include <stdlib.h>

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
static BufferWriter prepare_xtea_response(Connection *connection){
	if(connection->state != ConnectionState::Processing){
		LOG_ERR("Connection %s is not PROCESSING (State: %d)",
				connection->remote_address, (int)connection->state);
		close_connection(connection);
		return BufferWriter(NULL, 0);
	}

	BufferWriter write_buffer(connection->buffer, sizeof(connection->buffer));
	write_buffer.write_u16(0); // Encrypted Size
	write_buffer.write_u16(0); // Data Size
	return write_buffer;
}

static void send_xtea_response(Connection *connection, BufferWriter *write_buffer){
	if(connection->state != ConnectionState::Processing){
		LOG_ERR("Connection %s is not PROCESSING (State: %d)",
				connection->remote_address, (int)connection->state);
		close_connection(connection);
		return;
	}

	ASSERT(write_buffer != NULL
		&& write_buffer->buffer == connection->buffer
		&& write_buffer->size == sizeof(connection->buffer)
		&& write_buffer->position > 4);

	int data_size = write_buffer->position - 4;
	int encrypted_size = write_buffer->position - 2;
	while((encrypted_size % 8) != 0){
		write_buffer->write_u8(rand_r(&connection->random_seed));
		encrypted_size += 1;
	}

	if(write_buffer->overflowed()){
		LOG_ERR("Write buffer overflowed when writing response to %s",
				connection->remote_address);
		close_connection(connection);
		return;
	}

	write_buffer->rewrite_u16(0, encrypted_size);
	write_buffer->rewrite_u16(2, data_size);
	xtea_encrypt(connection->xtea,
			write_buffer->buffer + 2,
			write_buffer->position - 2);
	connection->state = ConnectionState::Writing;
	connection->rw_size = write_buffer->position;
	connection->rw_position = 0;
}

static void send_login_error(Connection *connection, const char *message){
	BufferWriter write_buffer = prepare_xtea_response(connection);
	write_buffer.write_u8(10); // LOGIN_ERROR
	write_buffer.write_string(message);
	send_xtea_response(connection, &write_buffer);
}

static void send_character_list(Connection *connection, int num_characters,
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

void process_login_request(Connection *connection, RsaKey& rsa_key,
		QueryClient& client, const ServerConfig& config){
	if(connection->rw_size != 145){
		LOG_ERR("Invalid login request size from %s (expected 145, got %d)",
				connection->remote_address, connection->rw_size);
		close_connection(connection);
		return;
	}

	BufferReader read_buffer(connection->buffer, connection->rw_size);
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
				connection->remote_address);
		close_connection(connection);
		return;
	}

	// IMPORTANT(fusion): Without a checksum, there is no way of validating
	// the asymmetric data. The best we can do is to verify that the first
	// plaintext byte is ZERO, but that alone isn't enough.
	if(!rsa_key.decrypt(asymmetric_data, sizeof(asymmetric_data)) || asymmetric_data[0] != 0){
		LOG_ERR("Failed to decrypt asymmetric data from %s",
				connection->remote_address);
		close_connection(connection);
		return;
	}

	read_buffer = BufferReader(asymmetric_data, sizeof(asymmetric_data));
	read_buffer.read_u8(); // always zero
	connection->xtea[0] = read_buffer.read_u32();
	connection->xtea[1] = read_buffer.read_u32();
	connection->xtea[2] = read_buffer.read_u32();
	connection->xtea[3] = read_buffer.read_u32();

	char email[51];
	char password[30];
	read_buffer.read_string(email, sizeof(email));
	read_buffer.read_string(password, sizeof(password));
	if(read_buffer.overflowed()){
		LOG_ERR("Malformed asymmetric data from %s", connection->remote_address);
		close_connection(connection);
		return;
	}

	if(email[0] == '\0'){
		send_login_error(connection, "You must enter an email address.");
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
			((connection->ip_address >> 24) & 0xFF),
			((connection->ip_address >> 16) & 0xFF),
			((connection->ip_address >>  8) & 0xFF),
			((connection->ip_address >>  0) & 0xFF));

	int num_characters = 0;
	int premium_days = 0;
	CharacterLoginData characters[50];
	int account_id = 0;
	int resolve_code = resolve_email(client, email, &account_id);
	if(resolve_code != 0){
		// Use same generic error as invalid credentials to avoid leaking
		// whether an email exists.
		send_login_error(connection, "Email or password is not correct.");
		return;
	}

	int login_code = login_account(client, account_id, password, ip_string,
			static_cast<int>(std::size(characters)), &num_characters, characters, &premium_days);
	switch(login_code){
		case 0:{
			send_character_list(connection, num_characters, characters, premium_days, config);
			break;
		}

		case 1:		// Invalid account
		case 2:{	// Invalid password
			send_login_error(connection, "Email or password is not correct.");
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
