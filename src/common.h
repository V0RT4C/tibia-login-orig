#ifndef TIBIA_COMMON_H_
#define TIBIA_COMMON_H_ 1

#include "common/types.h"
#include "common/assert.h"
#include "common/string_utils.h"
#include "common/logging.h"

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config/config.h"

extern ServerConfig g_config;

#include "common/byte_order.h"
#include "common/utf8.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"

#include "crypto/rsa.h"
#include "crypto/xtea.h"

// query.cc
//==============================================================================
enum {
	APPLICATION_TYPE_GAME	= 1,
	APPLICATION_TYPE_LOGIN	= 2,
	APPLICATION_TYPE_WEB	= 3,
};

enum {
	QUERY_STATUS_OK			= 0,
	QUERY_STATUS_ERROR		= 1,
	QUERY_STATUS_FAILED		= 3,
};

enum {
	QUERY_LOGIN				= 0,
	QUERY_LOGIN_ACCOUNT		= 11,
	QUERY_GET_WORLDS		= 150,
};

struct TQueryManagerConnection{
	int Socket;
};

struct TCharacterLoginData{
	char Name[30];
	char WorldName[30];
	int WorldAddress;
	int WorldPort;
};

struct TWorld {
	char Name[30];
	int Type;
	int NumPlayers;
	int MaxPlayers;
	int OnlinePeak;
	int OnlinePeakTimestamp;
	int LastStartup;
	int LastShutdown;
};

bool Connect(TQueryManagerConnection *Connection);
void Disconnect(TQueryManagerConnection *Connection);
bool IsConnected(TQueryManagerConnection *Connection);
BufferWriter PrepareQuery(int QueryType, uint8 *Buffer, int BufferSize);
int ExecuteQuery(TQueryManagerConnection *Connection, bool AutoReconnect,
		BufferWriter *WriteBuffer, BufferReader *OutReadBuffer);
int LoginAccount(int AccountID, const char *Password, const char *IPAddress,
		int MaxCharacters, int *NumCharacters, TCharacterLoginData *Characters,
		int *PremiumDays);
int GetWorld(const char *WorldName, TWorld *OutWorld);
bool InitQuery(void);
void ExitQuery(void);

// status.cc
//==============================================================================
const char *GetStatusString(void);

// connections.cc
//==============================================================================
enum ConnectionState {
	CONNECTION_FREE			= 0,
	CONNECTION_READING		= 1,
	CONNECTION_PROCESSING	= 2,
	CONNECTION_WRITING		= 3,
};

struct TConnection {
	ConnectionState State;
	int Socket;
	int IPAddress;
	int StartTime;
	int RWSize;
	int RWPosition;
	uint32 RandomSeed;
	uint32 XTEA[4];
	char RemoteAddress[32];
	uint8 Buffer[kb(2)];
};

struct TStatusRecord {
	int IPAddress;
	int Timestamp;
};

void ProcessConnections(void);
bool InitConnections(void);
void ExitConnections(void);
void ProcessLoginRequest(TConnection *Connection);
void ProcessStatusRequest(TConnection *Connection);

#endif //TIBIA_COMMON_H_
