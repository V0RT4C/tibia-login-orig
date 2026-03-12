#ifndef TIBIA_COMMON_H_
#define TIBIA_COMMON_H_ 1

#include "common/types.h"
#include "common/assert.h"

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ISPOW2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

#define LOG(...)		LogAdd("INFO", __VA_ARGS__)
#define LOG_WARN(...)	LogAddVerbose("WARN", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERR(...)	LogAddVerbose("ERR", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define PANIC(...)																\
	do{																			\
		LogAddVerbose("PANIC", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);	\
		TRAP();																	\
	}while(0)

struct TConfig {
	// Service Config
	int LoginPort;
	int ConnectionTimeout;
	int MaxConnections;
	int MaxStatusRecords;
	int MinStatusInterval;
	char QueryManagerHost[100];
	int QueryManagerPort;
	char QueryManagerPassword[30];

	// Service Info
	char StatusWorld[30];
	char Url[100];
	char Location[30];
	char ServerType[30];
	char ServerVersion[30];
	char ClientVersion[30];
	char Motd[256];
};

extern TConfig g_Config;

void LogAdd(const char *Prefix, const char *Format, ...) ATTR_PRINTF(2, 3);
void LogAddVerbose(const char *Prefix, const char *Function,
		const char *File, int Line, const char *Format, ...) ATTR_PRINTF(5, 6);

struct tm GetLocalTime(time_t t);
struct tm GetGMTime(time_t t);
int64 GetClockMonotonicMS(void);
int GetMonotonicUptime(void);

bool StringEmpty(const char *String);
bool StringEq(const char *A, const char *B);
bool StringEqCI(const char *A, const char *B);
bool StringCopy(char *Dest, int DestCapacity, const char *Src);
bool StringCopyN(char *Dest, int DestCapacity, const char *Src, int SrcLength);
bool StringFormat(char *Dest, int DestCapacity, const char *Format, ...) ATTR_PRINTF(3, 4);
bool StringFormatTime(char *Dest, int DestCapacity, const char *Format, int Timestamp);
void StringClear(char *Dest, int DestCapacity);
uint32 StringHash(const char *String);
bool StringEscape(char *Dest, int DestCapacity, const char *Src);


bool ParseBoolean(bool *Dest, const char *String);
bool ParseInteger(int *Dest, const char *String);
bool ParseDuration(int *Dest, const char *String);
bool ParseSize(int *Dest, const char *String);
bool ParseString(char *Dest, int DestCapacity, const char *String);
void ParseMotd(char *Dest, int DestCapacity, const char *String);
bool ReadConfig(const char *FileName, TConfig *Config);

// IMPORTANT(fusion): These macros should only be used when `Dest` is a char array
// to simplify the call to `StringCopy` where we'd use `sizeof(Dest)` to determine
// the size of the destination anyways.
#define StringBufCopy(Dest, Src)             StringCopy(Dest, sizeof(Dest), Src)
#define StringBufCopyN(Dest, Src, SrcLength) StringCopyN(Dest, sizeof(Dest), Src, SrcLength)
#define StringBufFormat(Dest, ...)           StringFormat(Dest, sizeof(Dest), __VA_ARGS__)
#define StringBufFormatTime(Dest, Format, Timestamp) \
		StringFormatTime(Dest, sizeof(Dest), Format, Timestamp)
#define StringBufClear(Dest)                 StringClear(Dest, sizeof(Dest));
#define StringBufEscape(Dest, Src)           StringEscape(Dest, sizeof(Dest), Src)
#define ParseStringBuf(Dest, String)         ParseString(Dest, sizeof(Dest), String)

#include "common/byte_order.h"
#include "common/utf8.h"
#include "common/buffer_reader.h"
#include "common/buffer_writer.h"

// crypto.cc
//==============================================================================
typedef void RSAKey;
RSAKey *RSALoadPEM(const char *FileName);
void RSAFree(RSAKey *Key);
bool RSADecrypt(RSAKey *Key, uint8 *Data, int Size);
void XTEAEncrypt(const uint32 *Key, uint8 *Data, int Size);
void XTEADecrypt(const uint32 *Key, uint8 *Data, int Size);

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
