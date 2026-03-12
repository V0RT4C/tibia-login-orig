#include "common.h"

#include <errno.h>
#include <signal.h>

int64   g_StartTimeMS    = 0;
int     g_ShutdownSignal = 0;
TConfig g_Config         = {};

void LogAdd(const char *Prefix, const char *Format, ...){
	char Entry[4096];
	va_list ap;
	va_start(ap, Format);
	vsnprintf(Entry, sizeof(Entry), Format, ap);
	va_end(ap);

	// NOTE(fusion): Trim trailing whitespace.
	int Length = (int)strlen(Entry);
	while(Length > 0 && isspace(Entry[Length - 1])){
		Entry[Length - 1] = 0;
		Length -= 1;
	}

	if(Length > 0){
		char TimeString[128];
		string_buf_format_time(TimeString, "%Y-%m-%d %H:%M:%S", (int)time(NULL));
		fprintf(stdout, "%s [%s] %s\n", TimeString, Prefix, Entry);
		fflush(stdout);
	}
}

void LogAddVerbose(const char *Prefix, const char *Function,
		const char *File, int Line, const char *Format, ...){
	char Entry[4096];
	va_list ap;
	va_start(ap, Format);
	vsnprintf(Entry, sizeof(Entry), Format, ap);
	va_end(ap);

	// NOTE(fusion): Trim trailing whitespace.
	int Length = (int)strlen(Entry);
	while(Length > 0 && isspace(Entry[Length - 1])){
		Entry[Length - 1] = 0;
		Length -= 1;
	}

	if(Length > 0){
		(void)File;
		(void)Line;
		char TimeString[128];
		string_buf_format_time(TimeString, "%Y-%m-%d %H:%M:%S", (int)time(NULL));
		fprintf(stdout, "%s [%s] %s: %s\n", TimeString, Prefix, Function, Entry);
		fflush(stdout);
	}
}

struct tm GetLocalTime(time_t t){
	struct tm result;
#if COMPILER_MSVC
	localtime_s(&result, &t);
#else
	localtime_r(&t, &result);
#endif
	return result;
}

struct tm GetGMTime(time_t t){
	struct tm result;
#if COMPILER_MSVC
	gmtime_s(&result, &t);
#else
	gmtime_r(&t, &result);
#endif
	return result;
}

int64 GetClockMonotonicMS(void){
#if OS_WINDOWS
	LARGE_INTEGER Counter, Frequency;
	QueryPerformanceCounter(&Counter);
	QueryPerformanceFrequency(&Frequency);
	return (int64)((Counter.QuadPart * 1000) / Frequency.QuadPart);
#else
	// NOTE(fusion): The coarse monotonic clock has a larger resolution but is
	// supposed to be faster, even avoiding system calls in some cases. It should
	// be fine for millisecond precision which is what we're using.
	struct timespec Time;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &Time);
	return ((int64)Time.tv_sec * 1000)
		+ ((int64)Time.tv_nsec / 1000000);
#endif
}

int GetMonotonicUptime(void){
	return (int)((GetClockMonotonicMS() - g_StartTimeMS) / 1000);
}

bool ParseBoolean(bool *Dest, const char *String){
	ASSERT(Dest && String);
	*Dest = string_equals_ignore_case(String, "true")
			|| string_equals_ignore_case(String, "on")
			|| string_equals_ignore_case(String, "yes");
	return *Dest
			|| string_equals_ignore_case(String, "false")
			|| string_equals_ignore_case(String, "off")
			|| string_equals_ignore_case(String, "no");
}

bool ParseInteger(int *Dest, const char *String){
	ASSERT(Dest && String);
	const char *StringEnd;
	*Dest = (int)strtol(String, (char**)&StringEnd, 0);
	return StringEnd > String;
}

bool ParseDuration(int *Dest, const char *String){
	ASSERT(Dest && String);
	const char *Suffix;
	*Dest = (int)strtol(String, (char**)&Suffix, 0);
	if(Suffix == String){
		return false;
	}

	while(Suffix[0] != 0 && isspace(Suffix[0])){
		Suffix += 1;
	}

	if(Suffix[0] == 'S' || Suffix[0] == 's'){
		*Dest *= (1);
	}else if(Suffix[0] == 'M' || Suffix[0] == 'm'){
		*Dest *= (60);
	}else if(Suffix[0] == 'H' || Suffix[0] == 'h'){
		*Dest *= (60 * 60);
	}

	return true;
}

bool ParseSize(int *Dest, const char *String){
	ASSERT(Dest && String);
	const char *Suffix;
	*Dest = (int)strtol(String, (char**)&Suffix, 0);
	if(Suffix == String){
		return false;
	}

	while(Suffix[0] != 0 && isspace(Suffix[0])){
		Suffix += 1;
	}

	if(Suffix[0] == 'K' || Suffix[0] == 'k'){
		*Dest *= (1024);
	}else if(Suffix[0] == 'M' || Suffix[0] == 'm'){
		*Dest *= (1024 * 1024);
	}

	return true;
}

bool ParseString(char *Dest, int DestCapacity, const char *String){
	ASSERT(Dest && DestCapacity > 0 && String);
	int StringStart = 0;
	int StringEnd = (int)strlen(String);
	if(StringEnd >= 2){
		if((String[0] == '"' && String[StringEnd - 1] == '"')
		|| (String[0] == '\'' && String[StringEnd - 1] == '\'')
		|| (String[0] == '`' && String[StringEnd - 1] == '`')){
			StringStart += 1;
			StringEnd -= 1;
		}
	}

	return string_copy_n(Dest, DestCapacity,
			&String[StringStart], (StringEnd - StringStart));
}

void ParseMotd(char *Dest, int DestCapacity, const char *String){
	char Motd[256];
	ParseString(Motd, DestCapacity, String);
	if(Motd[0] != 0){
		string_format(Dest, DestCapacity, "%u\n%s", string_hash(Motd), Motd);
	}
}

bool ReadConfig(const char *FileName, TConfig *Config){
	FILE *File = fopen(FileName, "rb");
	if(File == NULL){
		LOG_ERR("Failed to open config file \"%s\"", FileName);
		return false;
	}

	bool EndOfFile = false;
	for(int LineNumber = 1; !EndOfFile; LineNumber += 1){
		const int MaxLineSize = 1024;
		char Line[MaxLineSize];
		int LineSize = 0;
		int KeyStart = -1;
		int EqualPos = -1;
		while(true){
			int ch = fgetc(File);
			if(ch == EOF || ch == '\n'){
				if(ch == EOF){
					EndOfFile = true;
				}
				break;
			}

			if(LineSize < MaxLineSize){
				Line[LineSize] = (char)ch;
			}

			if(KeyStart == -1 && !isspace(ch)){
				KeyStart = LineSize;
			}

			if(EqualPos == -1 && ch == '='){
				EqualPos = LineSize;
			}

			LineSize += 1;
		}

		// NOTE(fusion): Check line size limit.
		if(LineSize > MaxLineSize){
			LOG_WARN("%s:%d: Exceeded line size limit of %d characters",
					FileName, LineNumber, MaxLineSize);
			continue;
		}

		// NOTE(fusion): Check empty line or comment.
		if(KeyStart == -1 || Line[KeyStart] == '#'){
			continue;
		}

		// NOTE(fusion): Check assignment.
		if(EqualPos == -1){
			LOG_WARN("%s:%d: No assignment found on non empty line",
					FileName, LineNumber);
			continue;
		}

		// NOTE(fusion): Check empty key.
		int KeyEnd = EqualPos;
		while(KeyEnd > KeyStart && isspace(Line[KeyEnd - 1])){
			KeyEnd -= 1;
		}

		if(KeyStart == KeyEnd){
			LOG_WARN("%s:%d: Empty key", FileName, LineNumber);
			continue;
		}

		// NOTE(fusion): Check empty value.
		int ValStart = EqualPos + 1;
		int ValEnd = LineSize;
		while(ValStart < ValEnd && isspace(Line[ValStart])){
			ValStart += 1;
		}

		while(ValEnd > ValStart && isspace(Line[ValEnd - 1])){
			ValEnd -= 1;
		}

		if(ValStart == ValEnd){
			LOG_WARN("%s:%d: Empty value", FileName, LineNumber);
			continue;
		}

		// NOTE(fusion): Parse KV pair.
		char Key[256];
		if(!string_buf_copy_n(Key, &Line[KeyStart], (KeyEnd - KeyStart))){
			LOG_WARN("%s:%d: Exceeded key size limit of %d characters",
					FileName, LineNumber, (int)(sizeof(Key) - 1));
			continue;
		}

		char Val[256];
		if(!string_buf_copy_n(Val, &Line[ValStart], (ValEnd - ValStart))){
			LOG_WARN("%s:%d: Exceeded value size limit of %d characters",
					FileName, LineNumber, (int)(sizeof(Val) - 1));
			continue;
		}

		if(string_equals_ignore_case(Key, "LoginPort")){
			ParseInteger(&Config->LoginPort, Val);
		}else if(string_equals_ignore_case(Key, "ConnectionTimeout")){
			ParseDuration(&Config->ConnectionTimeout, Val);
		}else if(string_equals_ignore_case(Key, "MaxConnections")){
			ParseInteger(&Config->MaxConnections, Val);
		}else if(string_equals_ignore_case(Key, "MaxStatusRecords")){
			ParseInteger(&Config->MaxStatusRecords, Val);
		}else if(string_equals_ignore_case(Key, "MinStatusInterval")){
			ParseDuration(&Config->MinStatusInterval, Val);
		}else if(string_equals_ignore_case(Key, "QueryManagerHost")){
			parse_string_buf(Config->QueryManagerHost, Val);
		}else if(string_equals_ignore_case(Key, "QueryManagerPort")){
			ParseInteger(&Config->QueryManagerPort, Val);
		}else if(string_equals_ignore_case(Key, "QueryManagerPassword")){
			parse_string_buf(Config->QueryManagerPassword, Val);
		}else if(string_equals_ignore_case(Key, "StatusWorld")){
			parse_string_buf(Config->StatusWorld, Val);
		}else if(string_equals_ignore_case(Key, "URL")){
			parse_string_buf(Config->Url, Val);
		}else if(string_equals_ignore_case(Key, "Location")){
			parse_string_buf(Config->Location, Val);
		}else if(string_equals_ignore_case(Key, "ServerType")){
			parse_string_buf(Config->ServerType, Val);
		}else if(string_equals_ignore_case(Key, "ServerVersion")){
			parse_string_buf(Config->ServerVersion, Val);
		}else if(string_equals_ignore_case(Key, "ClientVersion")){
			parse_string_buf(Config->ClientVersion, Val);
		}else if(string_equals_ignore_case(Key, "MOTD")){
			ParseMotd(Config->Motd, sizeof(Config->Motd), Val);
		}else{
			LOG_WARN("Unknown config \"%s\"", Key);
		}
	}

	fclose(File);
	return true;
}

static bool SigHandler(int SigNr, sighandler_t Handler){
	struct sigaction Action = {};
	Action.sa_handler = Handler;
	sigfillset(&Action.sa_mask);
	if(sigaction(SigNr, &Action, NULL) == -1){
		LOG_ERR("Failed to change handler for signal %d (%s): (%d) %s",
				SigNr, strsignal(SigNr), errno, strerror(errno));
		return false;
	}
	return true;
}

static void ShutdownHandler(int SigNr){
	g_ShutdownSignal = SigNr;
	//WakeConnections?
}

int main(int argc, const char **argv){
	(void)argc;
	(void)argv;

	g_StartTimeMS = GetClockMonotonicMS();
	g_ShutdownSignal = 0;
	if(!SigHandler(SIGPIPE, SIG_IGN)
	|| !SigHandler(SIGINT, ShutdownHandler)
	|| !SigHandler(SIGTERM, ShutdownHandler)){
		return EXIT_FAILURE;
	}

	// Service Config
	g_Config.LoginPort         = 7171;
	g_Config.ConnectionTimeout = 5;   // seconds
	g_Config.MaxConnections    = 10;
	g_Config.MaxStatusRecords  = 1024;
	g_Config.MinStatusInterval = 300; // seconds
	string_buf_copy(g_Config.QueryManagerHost, "127.0.0.1");
	g_Config.QueryManagerPort  = 7173;
	string_buf_copy(g_Config.QueryManagerPassword, "");

	// Service Info
	string_buf_copy(g_Config.StatusWorld,   "");
	string_buf_copy(g_Config.Url,           "");
	string_buf_copy(g_Config.Location,      "");
	string_buf_copy(g_Config.ServerType,    "");
	string_buf_copy(g_Config.ServerVersion, "");
	string_buf_copy(g_Config.ClientVersion, "");
	string_buf_copy(g_Config.Motd,          "");

	LOG("Tibia Login v0.2");
	if(!ReadConfig("config.cfg", &g_Config)){
		return EXIT_FAILURE;
	}

	LOG("Login port:          %d",     g_Config.LoginPort);
	LOG("Connection timeout:  %ds",    g_Config.ConnectionTimeout);
	LOG("Max connections:     %d",     g_Config.MaxConnections);
	LOG("Max status records:  %d",     g_Config.MaxStatusRecords);
	LOG("Min status interval: %ds",    g_Config.MinStatusInterval);
	LOG("Query manager host:  \"%s\"", g_Config.QueryManagerHost);
	LOG("Query manager port:  %d",     g_Config.QueryManagerPort);
	LOG("Status world:        \"%s\"", g_Config.StatusWorld);
	LOG("URL:                 \"%s\"", g_Config.Url);
	LOG("Location:            \"%s\"", g_Config.Location);
	LOG("Server type:         \"%s\"", g_Config.ServerType);
	LOG("Server version:      \"%s\"", g_Config.ServerVersion);
	LOG("Client version:      \"%s\"", g_Config.ClientVersion);

	{	// NOTE(fusion): Print MOTD preview with escape codes.
		char MotdPreview[30];
		if(string_buf_escape(MotdPreview, g_Config.Motd)){
			LOG("MOTD:                \"%s\"", MotdPreview);
		}else{
			LOG("MOTD:                \"%s...\"", MotdPreview);
		}
	}

	atexit(ExitQuery);
	atexit(ExitConnections);
	if(!InitQuery() || !InitConnections()){
		return EXIT_FAILURE;
	}

	LOG("Running...");
	while(g_ShutdownSignal == 0){
		ProcessConnections();
	}

	LOG("Received signal %d (%s), shutting down...",
			g_ShutdownSignal, strsignal(g_ShutdownSignal));
	return EXIT_SUCCESS;
}

