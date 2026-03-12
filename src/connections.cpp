#include "common.h"
#include "login/login_handler.h"

#include <errno.h>
#include <memory>
#include <vector>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

static std::unique_ptr<RsaKey> g_PrivateKey;

static int g_Listener = -1;
static TConnection *g_Connections;
static int g_MaxConnections;

static TStatusRecord *g_StatusRecords;
static int g_MaxStatusRecords;

// Connection Handling
//==============================================================================
static int ListenerBind(uint16 Port){
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

static int ListenerAccept(int Listener, uint32 *OutAddr, uint16 *OutPort){
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

void CloseConnection(TConnection *Connection){
	if(Connection->Socket != -1){
		close(Connection->Socket);
		Connection->Socket = -1;
	}
}

static TConnection *AssignConnection(int Socket, uint32 Addr, uint16 Port){
	int ConnectionIndex = -1;
	for(int i = 0; i < g_config.max_connections; i += 1){
		if(g_Connections[i].State == CONNECTION_FREE){
			ConnectionIndex = i;
			break;
		}
	}

	TConnection *Connection = NULL;
	if(ConnectionIndex != -1){
		Connection = &g_Connections[ConnectionIndex];
		Connection->State = CONNECTION_READING;
		Connection->Socket = Socket;
		Connection->IPAddress = (int)Addr;
		Connection->StartTime = get_monotonic_uptime();
		Connection->RandomSeed = (uint32)rand();
		string_buf_format(Connection->RemoteAddress,
				"%d.%d.%d.%d:%d",
				((Connection->IPAddress >> 24) & 0xFF),
				((Connection->IPAddress >> 16) & 0xFF),
				((Connection->IPAddress >>  8) & 0xFF),
				((Connection->IPAddress >>  0) & 0xFF),
				(int)Port);
		LOG("Connection %s assigned to slot %d",
				Connection->RemoteAddress, ConnectionIndex);
	}
	return Connection;
}

static void ReleaseConnection(TConnection *Connection){
	if(Connection->State != CONNECTION_FREE){
		LOG("Connection %s released", Connection->RemoteAddress);
		CloseConnection(Connection);
		memset(Connection, 0, sizeof(TConnection));
		Connection->State = CONNECTION_FREE;
	}
}

static void CheckConnectionInput(TConnection *Connection, int Events){
	if(Connection->Socket == -1 || (Events & POLLIN) == 0){
		return;
	}

	if(Connection->State != CONNECTION_READING){
		LOG_ERR("Connection %s (State: %d) sending out-of-order data",
				Connection->RemoteAddress, Connection->State);
		CloseConnection(Connection);
		return;
	}

	while(true){
		int ReadSize = (Connection->RWSize > 0 ? Connection->RWSize : 2);
		int BytesRead = (int)read(Connection->Socket,
				(Connection->Buffer + Connection->RWPosition),
				(ReadSize           - Connection->RWPosition));
		if(BytesRead == -1){
			if(errno != EAGAIN){
				// NOTE(fusion): Connection error.
				CloseConnection(Connection);
			}
			break;
		}else if(BytesRead == 0){
			// NOTE(fusion): Graceful close.
			CloseConnection(Connection);
			break;
		}

		Connection->RWPosition += BytesRead;
		if(Connection->RWPosition >= ReadSize){
			if(Connection->RWSize != 0){
				Connection->State = CONNECTION_PROCESSING;
				break;
			}else if(Connection->RWPosition == 2){
				int PayloadSize = (int)read_u16_le(Connection->Buffer);
				if(PayloadSize <= 0 || PayloadSize > (int)sizeof(Connection->Buffer)){
					CloseConnection(Connection);
					break;
				}

				Connection->RWSize = PayloadSize;
				Connection->RWPosition = 0;
			}else{
				PANIC("Invalid input state (State: %d, RWSize: %d, RWPosition: %d)",
						Connection->State, Connection->RWSize, Connection->RWPosition);
			}
		}
	}
}

static void CheckConnectionRequest(TConnection *Connection){
	if(Connection->Socket == -1){
		return;
	}

	if(Connection->State != CONNECTION_PROCESSING){
		return;
	}

	// PARANOID(fusion): A non-empty payload is guaranteed, due to how we parse
	// input in `CheckConnectionInput` just above.
	ASSERT(Connection->RWSize > 0);

	int Command = Connection->Buffer[0];
	if(Command == 1){
		process_login_request(Connection, *g_PrivateKey, *query_client, g_config);
	}else if(Command == 255){
		ProcessStatusRequest(Connection);
	}else{
		LOG_ERR("Invalid command %d from %s (expected 1 or 255)",
				Command, Connection->RemoteAddress);
		CloseConnection(Connection);
	}
}

static void CheckConnectionOutput(TConnection *Connection, int Events){
	// NOTE(fusion): We're only polling `POLLOUT` when the connection is WRITING,
	// but we want to allow requests to complete in a single cycle, so we always
	// check for output if the connection is WRITING.
	(void)Events;

	if(Connection->Socket == -1){
		return;
	}

	if(Connection->State != CONNECTION_WRITING){
		return;
	}

	while(true){
		int BytesWritten = (int)write(Connection->Socket,
				(Connection->Buffer + Connection->RWPosition),
				(Connection->RWSize - Connection->RWPosition));
		if(BytesWritten == -1){
			if(errno != EAGAIN){
				CloseConnection(Connection);
			}
			break;
		}

		Connection->RWPosition += BytesWritten;
		if(Connection->RWPosition >= Connection->RWSize){
			CloseConnection(Connection);
			break;
		}
	}
}

static void CheckConnection(TConnection *Connection, int Events){
	ASSERT((Events & POLLNVAL) == 0);

	if((Events & (POLLERR | POLLHUP)) != 0){
		CloseConnection(Connection);
	}

	if(g_config.connection_timeout > 0){
		int ElapsedTime = get_monotonic_uptime() - Connection->StartTime;
		if(ElapsedTime >= g_config.connection_timeout){
			LOG_WARN("Connection %s TIMEDOUT (ElapsedTime: %ds, Timeout: %ds)",
					Connection->RemoteAddress, ElapsedTime, g_config.connection_timeout);
			CloseConnection(Connection);
		}
	}

	if(Connection->Socket == -1){
		ReleaseConnection(Connection);
	}
}

static void AcceptConnections(int Events){
	ASSERT(g_Listener != -1);
	if((Events & POLLIN) == 0){
		return;
	}

	while(true){
		uint32 Addr;
		uint16 Port;
		int Socket = ListenerAccept(g_Listener, &Addr, &Port);
		if(Socket == -1){
			break;
		}

		if(AssignConnection(Socket, Addr, Port) == NULL){
			LOG_ERR("Rejecting connection %08X:%d:"
					" max number of connections reached (%d)",
					Addr, Port, g_config.max_connections);
			close(Socket);
		}
	}
}

void ProcessConnections(void){
	int NumFds = 0;
	int MaxFds = g_MaxConnections + 1;
	std::vector<pollfd> Fds(MaxFds);
	std::vector<int> ConnectionIndices(MaxFds);

	if(g_Listener != -1){
		Fds[NumFds].fd = g_Listener;
		Fds[NumFds].events = POLLIN;
		Fds[NumFds].revents = 0;
		ConnectionIndices[NumFds] = -1;
		NumFds += 1;
	}

	for(int i = 0; i < g_config.max_connections; i += 1){
		if(g_Connections[i].State == CONNECTION_FREE){
			continue;
		}

		Fds[NumFds].fd = g_Connections[i].Socket;
		Fds[NumFds].events = POLLIN;
		if(g_Connections[i].State == CONNECTION_WRITING){
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
			TConnection *Connection = &g_Connections[Index];
			CheckConnectionInput(Connection, Events);
			CheckConnectionRequest(Connection);
			CheckConnectionOutput(Connection, Events);
			CheckConnection(Connection, Events);
		}else if(Index == -1 && Fds[i].fd == g_Listener){
			AcceptConnections(Events);
		}else{
			LOG_ERR("Unknown connection index %d", Index);
		}
	}
}

bool InitConnections(void){
	ASSERT(g_PrivateKey == nullptr);
	ASSERT(g_Listener == -1);
	ASSERT(g_Connections == NULL);
	ASSERT(g_StatusRecords == NULL);

	g_PrivateKey = RsaKey::from_pem_file("tibia.pem");
	if(g_PrivateKey == nullptr){
		LOG_ERR("Failed to load RSA key");
		return false;
	}

	g_Listener = ListenerBind((uint16)g_config.login_port);
	if(g_Listener == -1){
		LOG_ERR("Failed to bind listener to port %d", g_config.login_port);
		return false;
	}

	g_MaxConnections = g_config.max_connections;
	g_Connections = (TConnection*)calloc(
			g_MaxConnections, sizeof(TConnection));
	for(int i = 0; i < g_MaxConnections; i += 1){
		g_Connections[i].State = CONNECTION_FREE;
	}

	g_MaxStatusRecords = g_config.max_status_records;
	g_StatusRecords = (TStatusRecord*)calloc(
			g_MaxStatusRecords, sizeof(TStatusRecord));

	return true;
}

void ExitConnections(void){
	g_PrivateKey.reset();

	if(g_Listener != -1){
		close(g_Listener);
		g_Listener = -1;
	}

	if(g_Connections != NULL){
		for(int i = 0; i < g_MaxConnections; i += 1){
			ReleaseConnection(&g_Connections[i]);
		}

		free(g_Connections);
		g_Connections = NULL;
	}

	if(g_StatusRecords != NULL){
		free(g_StatusRecords);
		g_StatusRecords = NULL;
	}
}

// Status Request
//==============================================================================
static bool AllowStatusRequest(int IPAddress){
	TStatusRecord *Record = NULL;
	int LeastRecentlyUsedIndex = 0;
	int LeastRecentlyUsedTime = g_StatusRecords[0].Timestamp;
	int TimeNow = get_monotonic_uptime();
	for(int i = 0; i < g_MaxStatusRecords; i += 1){
		if(g_StatusRecords[i].Timestamp < LeastRecentlyUsedTime){
			LeastRecentlyUsedIndex = i;
			LeastRecentlyUsedTime = g_StatusRecords[i].Timestamp;
		}

		if(g_StatusRecords[i].IPAddress == IPAddress){
			Record = &g_StatusRecords[i];
			break;
		}
	}

	bool Result = false;
	if(Record == NULL){
		Record = &g_StatusRecords[LeastRecentlyUsedIndex];
		Record->IPAddress = IPAddress;
		Record->Timestamp = TimeNow;
		Result = true;
	}else if((TimeNow - Record->Timestamp) >= g_config.min_status_interval){
		Record->Timestamp = TimeNow;
		Result = true;
	}

	return Result;
}

static void SendStatusString(TConnection *Connection, const char *StatusString){
	if(Connection->State != CONNECTION_PROCESSING){
		LOG_ERR("Connection %s is not PROCESSING (State: %d)",
				Connection->RemoteAddress, Connection->State);
		CloseConnection(Connection);
		return;
	}

	int Length = (int)strlen(StatusString);
	if(Length > (int)sizeof(Connection->Buffer)){
		Length = (int)sizeof(Connection->Buffer);
	}

	Connection->RWSize = Length;
	Connection->RWPosition = 0;
	memcpy(Connection->Buffer, StatusString, Length);
	Connection->State = CONNECTION_WRITING;
}

void ProcessStatusRequest(TConnection *Connection){
	if(!AllowStatusRequest(Connection->IPAddress)){
		LOG_ERR("Too many status requests from %s", Connection->RemoteAddress);
		CloseConnection(Connection);
		return;
	}

	BufferReader ReadBuffer(Connection->Buffer, Connection->RWSize);
	ReadBuffer.read_u8(); // always 255 for a status request
	int Format = (int)ReadBuffer.read_u8();
	if(Format == 255){ // XML
		char Request[5] = {};
		ReadBuffer.read_bytes((uint8*)Request, 4);
		if(string_equals_ignore_case(Request, "info")){
			const char *StatusString = get_status_string(*query_client, g_config);
			SendStatusString(Connection, StatusString);
		}else{
			LOG_WARN("Invalid status request \"%s\" from %s",
					Request, Connection->RemoteAddress);
			CloseConnection(Connection);
		}
	}else{
		LOG_WARN("Invalid status format %d from %s",
				Format, Connection->RemoteAddress);
		CloseConnection(Connection);
	}
}

