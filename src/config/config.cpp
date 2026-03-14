#include "config.h"
#include "../common/string_utils.h"
#include "../common/logging.h"

#include <cstdlib>
#include <cstring>

void parse_motd(char *dest, int dest_capacity, const char *str){
	char motd_buf[256];
	// Copy str directly — no quote stripping needed for env var values
	string_copy(motd_buf, (int)sizeof(motd_buf), str);
	if(motd_buf[0] != 0){
		string_format(dest, dest_capacity, "%u\n%s", string_hash(motd_buf), motd_buf);
	}
}

static const char *EnvStr(const char *Name, const char *Default){
	const char *Value = getenv(Name);
	return (Value != NULL) ? Value : Default;
}

static int EnvInt(const char *Name, int Default){
	const char *Value = getenv(Name);
	return (Value != NULL) ? atoi(Value) : Default;
}

bool read_config(ServerConfig *config){
	// Service Config
	config->login_port         = EnvInt("SENJA_LOGIN_PORT", 7171);
	config->connection_timeout = EnvInt("SENJA_CONNECTION_TIMEOUT", 5);
	config->max_connections    = EnvInt("SENJA_MAX_CONNECTIONS", 10);
	config->max_status_records  = EnvInt("SENJA_MAX_STATUS_RECORDS", 1024);
	config->min_status_interval = EnvInt("SENJA_MIN_STATUS_INTERVAL", 300);

	string_buf_copy(config->query_manager_host, EnvStr("SENJA_QM_HOST", "127.0.0.1"));
	config->query_manager_port = EnvInt("SENJA_QM_PORT", 7173);

	const char *QmPassword = getenv("SENJA_QM_PASSWORD");
	if(QmPassword == NULL){
		LOG_ERR("SENJA_QM_PASSWORD environment variable is required");
		return false;
	}
	string_buf_copy(config->query_manager_password, QmPassword);

	// Service Info
	string_buf_copy(config->status_world,   EnvStr("SENJA_WORLD_NAME", "Senja"));
	string_buf_copy(config->url,           EnvStr("SENJA_URL", ""));
	string_buf_copy(config->location,      EnvStr("SENJA_LOCATION", ""));
	string_buf_copy(config->server_type,    EnvStr("SENJA_SERVER_TYPE", "Tibia"));
	string_buf_copy(config->server_version, EnvStr("SENJA_SERVER_VERSION", "7.7"));
	string_buf_copy(config->client_version, EnvStr("SENJA_CLIENT_VERSION", "7.7"));

	const char *Motd = EnvStr("SENJA_MOTD", "Welcome to Tibia!");
	config->motd[0] = '\0';
	if(Motd[0] != '\0'){
		parse_motd(config->motd, sizeof(config->motd), Motd);
	}

	// Transport
	const char *Mode = EnvStr("SENJA_TRANSPORT_MODE", "both");
	if(string_equals_ignore_case(Mode, "tcp")){
		config->transport_mode = TRANSPORT_TCP;
	}else if(string_equals_ignore_case(Mode, "websocket")){
		config->transport_mode = TRANSPORT_WEBSOCKET;
	}else{
		config->transport_mode = TRANSPORT_BOTH;
	}
	config->websocket_port = EnvInt("SENJA_WEBSOCKET_PORT", 9172);
	string_copy(config->websocket_address, (int)sizeof(config->websocket_address),
			EnvStr("SENJA_WEBSOCKET_ADDRESS", "0.0.0.0"));

	return true;
}
