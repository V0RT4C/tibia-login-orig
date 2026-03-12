#include "network/server.h"
#include "network/connection_types.h"
#include "common.h"
#include "login/login_handler.h"
#include "status/status_handler.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <vector>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

static std::unique_ptr<RsaKey> g_private_key;
static int g_listener = -1;
static Connection* g_connections;
static int g_max_connections;
static StatusRecord* g_status_records;
static int g_max_status_records;

// Connection Handling
//==============================================================================
static int listener_bind(uint16 Port){
	int Socket = socket(AF_INET, SOCK_STREAM, 0);
	if(Socket == -1){
		LOG_ERR("Failed to create listener socket: (%d) %s", errno, strerror(errno));
		return -1;
	}

	int ReuseAddr = 1;
	if(setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, &ReuseAddr, sizeof(ReuseAddr)) == -1){
		LOG_ERR("Failed to set SO_REUSADDR: (%d) %s", errno, strerror(errno));
		close(Socket);
		return -1;
	}

	int Flags = fcntl(Socket, F_GETFL);
	if(Flags == -1){
		LOG_ERR("Failed to get socket flags: (%d) %s", errno, strerror(errno));
		close(Socket);
		return -1;
	}

	if(fcntl(Socket, F_SETFL, Flags | O_NONBLOCK) == -1){
		LOG_ERR("Failed to set socket flags: (%d) %s", errno, strerror(errno));
		close(Socket);
		return -1;
	}

	sockaddr_in Addr = {};
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(Port);
	Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(Socket, (sockaddr*)&Addr, sizeof(Addr)) == -1){
		LOG_ERR("Failed to bind socket to port %d: (%d) %s", Port, errno, strerror(errno));
		close(Socket);
		return -1;
	}

	if(listen(Socket, 128) == -1){
		LOG_ERR("Failed to listen to port %d: (%d) %s", Port, errno, strerror(errno));
		close(Socket);
		return -1;
	}

	return Socket;
}

static int listener_accept(int Listener, uint32 *OutAddr, uint16 *OutPort){
	while(true){
		sockaddr_in SocketAddr = {};
		socklen_t SocketAddrLen = sizeof(SocketAddr);
		int Socket = accept(Listener, (sockaddr*)&SocketAddr, &SocketAddrLen);
		if(Socket == -1){
			if(errno != EAGAIN){
				LOG_ERR("Failed to accept connection: (%d) %s", errno, strerror(errno));
			}
			return -1;
		}

		int Flags = fcntl(Socket, F_GETFL);
		if(Flags == -1){
			LOG_ERR("Failed to get socket flags: (%d) %s", errno, strerror(errno));
			close(Socket);
			continue;
		}

		if(fcntl(Socket, F_SETFL, Flags | O_NONBLOCK) == -1){
			LOG_ERR("Failed to set socket flags: (%d) %s", errno, strerror(errno));
			close(Socket);
			continue;
		}

		if(OutAddr){
			*OutAddr = ntohl(SocketAddr.sin_addr.s_addr);
		}

		if(OutPort){
			*OutPort = ntohs(SocketAddr.sin_port);
		}

		return Socket;
	}
}

void close_connection(Connection *connection){
	if(connection->socket != -1){
		close(connection->socket);
		connection->socket = -1;
	}
}

static Connection *assign_connection(int Socket, uint32 Addr, uint16 Port){
	int ConnectionIndex = -1;
	for(int i = 0; i < g_config.max_connections; i += 1){
		if(g_connections[i].state == ConnectionState::Free){
			ConnectionIndex = i;
			break;
		}
	}

	Connection *connection = NULL;
	if(ConnectionIndex != -1){
		connection = &g_connections[ConnectionIndex];
		connection->state = ConnectionState::Reading;
		connection->socket = Socket;
		connection->ip_address = (int)Addr;
		connection->start_time = get_monotonic_uptime();
		connection->random_seed = (uint32)rand();
		string_buf_format(connection->remote_address,
				"%d.%d.%d.%d:%d",
				((connection->ip_address >> 24) & 0xFF),
				((connection->ip_address >> 16) & 0xFF),
				((connection->ip_address >>  8) & 0xFF),
				((connection->ip_address >>  0) & 0xFF),
				(int)Port);
		LOG("Connection %s assigned to slot %d",
				connection->remote_address, ConnectionIndex);
	}
	return connection;
}

static void release_connection(Connection *connection){
	if(connection->state != ConnectionState::Free){
		LOG("Connection %s released", connection->remote_address);
		close_connection(connection);
		memset(connection, 0, sizeof(Connection));
		connection->state = ConnectionState::Free;
	}
}

static void check_connection_input(Connection *connection, int Events){
	if(connection->socket == -1 || (Events & POLLIN) == 0){
		return;
	}

	if(connection->state != ConnectionState::Reading){
		LOG_ERR("Connection %s (State: %d) sending out-of-order data",
				connection->remote_address, (int)connection->state);
		close_connection(connection);
		return;
	}

	while(true){
		int ReadSize = (connection->rw_size > 0 ? connection->rw_size : 2);
		int BytesRead = (int)read(connection->socket,
				(connection->buffer + connection->rw_position),
				(ReadSize           - connection->rw_position));
		if(BytesRead == -1){
			if(errno != EAGAIN){
				// NOTE(fusion): Connection error.
				close_connection(connection);
			}
			break;
		}else if(BytesRead == 0){
			// NOTE(fusion): Graceful close.
			close_connection(connection);
			break;
		}

		connection->rw_position += BytesRead;
		if(connection->rw_position >= ReadSize){
			if(connection->rw_size != 0){
				connection->state = ConnectionState::Processing;
				break;
			}else if(connection->rw_position == 2){
				int PayloadSize = (int)read_u16_le(connection->buffer);
				if(PayloadSize <= 0 || PayloadSize > (int)sizeof(connection->buffer)){
					close_connection(connection);
					break;
				}

				connection->rw_size = PayloadSize;
				connection->rw_position = 0;
			}else{
				PANIC("Invalid input state (State: %d, RWSize: %d, RWPosition: %d)",
						(int)connection->state, connection->rw_size, connection->rw_position);
			}
		}
	}
}

static void process_status_request(Connection *connection);

static void check_connection_request(Connection *connection){
	if(connection->socket == -1){
		return;
	}

	if(connection->state != ConnectionState::Processing){
		return;
	}

	// PARANOID(fusion): A non-empty payload is guaranteed, due to how we parse
	// input in `check_connection_input` just above.
	ASSERT(connection->rw_size > 0);

	int Command = connection->buffer[0];
	if(Command == 1){
		process_login_request(connection, *g_private_key, *query_client, g_config);
	}else if(Command == 255){
		process_status_request(connection);
	}else{
		LOG_ERR("Invalid command %d from %s (expected 1 or 255)",
				Command, connection->remote_address);
		close_connection(connection);
	}
}

static void check_connection_output(Connection *connection, int Events){
	// NOTE(fusion): We're only polling `POLLOUT` when the connection is WRITING,
	// but we want to allow requests to complete in a single cycle, so we always
	// check for output if the connection is WRITING.
	(void)Events;

	if(connection->socket == -1){
		return;
	}

	if(connection->state != ConnectionState::Writing){
		return;
	}

	while(true){
		int BytesWritten = (int)write(connection->socket,
				(connection->buffer + connection->rw_position),
				(connection->rw_size - connection->rw_position));
		if(BytesWritten == -1){
			if(errno != EAGAIN){
				close_connection(connection);
			}
			break;
		}

		connection->rw_position += BytesWritten;
		if(connection->rw_position >= connection->rw_size){
			close_connection(connection);
			break;
		}
	}
}

static void check_connection(Connection *connection, int Events){
	ASSERT((Events & POLLNVAL) == 0);

	if((Events & (POLLERR | POLLHUP)) != 0){
		close_connection(connection);
	}

	if(g_config.connection_timeout > 0){
		int ElapsedTime = get_monotonic_uptime() - connection->start_time;
		if(ElapsedTime >= g_config.connection_timeout){
			LOG_WARN("Connection %s TIMEDOUT (ElapsedTime: %ds, Timeout: %ds)",
					connection->remote_address, ElapsedTime, g_config.connection_timeout);
			close_connection(connection);
		}
	}

	if(connection->socket == -1){
		release_connection(connection);
	}
}

static void accept_connections(int Events){
	ASSERT(g_listener != -1);
	if((Events & POLLIN) == 0){
		return;
	}

	while(true){
		uint32 Addr;
		uint16 Port;
		int Socket = listener_accept(g_listener, &Addr, &Port);
		if(Socket == -1){
			break;
		}

		if(assign_connection(Socket, Addr, Port) == NULL){
			LOG_ERR("Rejecting connection %08X:%d:"
					" max number of connections reached (%d)",
					Addr, Port, g_config.max_connections);
			close(Socket);
		}
	}
}

void process_connections(void){
	int NumFds = 0;
	int MaxFds = g_max_connections + 1;
	std::vector<pollfd> Fds(MaxFds);
	std::vector<int> ConnectionIndices(MaxFds);

	if(g_listener != -1){
		Fds[NumFds].fd = g_listener;
		Fds[NumFds].events = POLLIN;
		Fds[NumFds].revents = 0;
		ConnectionIndices[NumFds] = -1;
		NumFds += 1;
	}

	for(int i = 0; i < g_config.max_connections; i += 1){
		if(g_connections[i].state == ConnectionState::Free){
			continue;
		}

		Fds[NumFds].fd = g_connections[i].socket;
		Fds[NumFds].events = POLLIN;
		if(g_connections[i].state == ConnectionState::Writing){
			Fds[NumFds].events |= POLLOUT;
		}
		Fds[NumFds].revents = 0;
		ConnectionIndices[NumFds] = i;
		NumFds += 1;
	}

	// NOTE(fusion): Block for 1 second at most, so we can properly timeout
	// idle connections.
	ASSERT(NumFds > 0);
	int NumEvents = poll(Fds.data(), NumFds, 1000);
	if(NumEvents == -1){
		if(errno != ETIMEDOUT && errno != EINTR){
			LOG_ERR("Failed to poll connections: (%d) %s",
					errno, strerror(errno));
		}
		return;
	}

	// NOTE(fusion): Process connections.
	for(int i = 0; i < NumFds; i += 1){
		int Index = ConnectionIndices[i];
		int Events = (int)Fds[i].revents;
		if(Index >= 0 && Index < g_config.max_connections){
			Connection *connection = &g_connections[Index];
			check_connection_input(connection, Events);
			check_connection_request(connection);
			check_connection_output(connection, Events);
			check_connection(connection, Events);
		}else if(Index == -1 && Fds[i].fd == g_listener){
			accept_connections(Events);
		}else{
			LOG_ERR("Unknown connection index %d", Index);
		}
	}
}

bool init_connections(void){
	ASSERT(g_private_key == nullptr);
	ASSERT(g_listener == -1);
	ASSERT(g_connections == NULL);
	ASSERT(g_status_records == NULL);

	g_private_key = RsaKey::from_pem_file("tibia.pem");
	if(g_private_key == nullptr){
		LOG_ERR("Failed to load RSA key");
		return false;
	}

	g_listener = listener_bind((uint16)g_config.login_port);
	if(g_listener == -1){
		LOG_ERR("Failed to bind listener to port %d", g_config.login_port);
		return false;
	}

	g_max_connections = g_config.max_connections;
	g_connections = (Connection*)calloc(
			g_max_connections, sizeof(Connection));
	for(int i = 0; i < g_max_connections; i += 1){
		g_connections[i].state = ConnectionState::Free;
	}

	g_max_status_records = g_config.max_status_records;
	g_status_records = (StatusRecord*)calloc(
			g_max_status_records, sizeof(StatusRecord));

	return true;
}

void exit_connections(void){
	g_private_key.reset();

	if(g_listener != -1){
		close(g_listener);
		g_listener = -1;
	}

	if(g_connections != NULL){
		for(int i = 0; i < g_max_connections; i += 1){
			release_connection(&g_connections[i]);
		}

		free(g_connections);
		g_connections = NULL;
	}

	if(g_status_records != NULL){
		free(g_status_records);
		g_status_records = NULL;
	}
}

// Status Request
//==============================================================================
static bool allow_status_request(int ip_address){
	StatusRecord *Record = NULL;
	int LeastRecentlyUsedIndex = 0;
	int LeastRecentlyUsedTime = g_status_records[0].timestamp;
	int TimeNow = get_monotonic_uptime();
	for(int i = 0; i < g_max_status_records; i += 1){
		if(g_status_records[i].timestamp < LeastRecentlyUsedTime){
			LeastRecentlyUsedIndex = i;
			LeastRecentlyUsedTime = g_status_records[i].timestamp;
		}

		if(g_status_records[i].ip_address == ip_address){
			Record = &g_status_records[i];
			break;
		}
	}

	bool Result = false;
	if(Record == NULL){
		Record = &g_status_records[LeastRecentlyUsedIndex];
		Record->ip_address = ip_address;
		Record->timestamp = TimeNow;
		Result = true;
	}else if((TimeNow - Record->timestamp) >= g_config.min_status_interval){
		Record->timestamp = TimeNow;
		Result = true;
	}

	return Result;
}

static void send_status_string(Connection *connection, const char *StatusString){
	if(connection->state != ConnectionState::Processing){
		LOG_ERR("Connection %s is not PROCESSING (State: %d)",
				connection->remote_address, (int)connection->state);
		close_connection(connection);
		return;
	}

	int Length = (int)strlen(StatusString);
	if(Length > (int)sizeof(connection->buffer)){
		Length = (int)sizeof(connection->buffer);
	}

	connection->rw_size = Length;
	connection->rw_position = 0;
	memcpy(connection->buffer, StatusString, Length);
	connection->state = ConnectionState::Writing;
}

static void process_status_request(Connection *connection){
	if(!allow_status_request(connection->ip_address)){
		LOG_ERR("Too many status requests from %s", connection->remote_address);
		close_connection(connection);
		return;
	}

	BufferReader ReadBuffer(connection->buffer, connection->rw_size);
	ReadBuffer.read_u8(); // always 255 for a status request
	int Format = (int)ReadBuffer.read_u8();
	if(Format == 255){ // XML
		char Request[5] = {};
		ReadBuffer.read_bytes((uint8*)Request, 4);
		if(string_equals_ignore_case(Request, "info")){
			const char *StatusString = get_status_string(*query_client, g_config);
			send_status_string(connection, StatusString);
		}else{
			LOG_WARN("Invalid status request \"%s\" from %s",
					Request, connection->remote_address);
			close_connection(connection);
		}
	}else{
		LOG_WARN("Invalid status format %d from %s",
				Format, connection->remote_address);
		close_connection(connection);
	}
}
