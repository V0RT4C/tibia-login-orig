#include "config.h"
#include "../common/string_utils.h"
#include "../common/logging.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

bool parse_boolean(bool *dest, const char *str){
	ASSERT(dest && str);
	*dest = string_equals_ignore_case(str, "true")
			|| string_equals_ignore_case(str, "on")
			|| string_equals_ignore_case(str, "yes");
	return *dest
			|| string_equals_ignore_case(str, "false")
			|| string_equals_ignore_case(str, "off")
			|| string_equals_ignore_case(str, "no");
}

bool parse_integer(int *dest, const char *str){
	ASSERT(dest && str);
	const char *str_end;
	*dest = (int)strtol(str, (char**)&str_end, 0);
	return str_end > str;
}

bool parse_duration(int *dest, const char *str){
	ASSERT(dest && str);
	const char *suffix;
	*dest = (int)strtol(str, (char**)&suffix, 0);
	if(suffix == str){
		return false;
	}

	while(suffix[0] != 0 && isspace(suffix[0])){
		suffix += 1;
	}

	if(suffix[0] == 'S' || suffix[0] == 's'){
		*dest *= (1);
	}else if(suffix[0] == 'M' || suffix[0] == 'm'){
		*dest *= (60);
	}else if(suffix[0] == 'H' || suffix[0] == 'h'){
		*dest *= (60 * 60);
	}

	return true;
}

bool parse_size(int *dest, const char *str){
	ASSERT(dest && str);
	const char *suffix;
	*dest = (int)strtol(str, (char**)&suffix, 0);
	if(suffix == str){
		return false;
	}

	while(suffix[0] != 0 && isspace(suffix[0])){
		suffix += 1;
	}

	if(suffix[0] == 'K' || suffix[0] == 'k'){
		*dest *= (1024);
	}else if(suffix[0] == 'M' || suffix[0] == 'm'){
		*dest *= (1024 * 1024);
	}

	return true;
}

bool parse_string(char *dest, int dest_capacity, const char *str){
	ASSERT(dest && dest_capacity > 0 && str);
	int str_start = 0;
	int str_end = (int)strlen(str);
	if(str_end >= 2){
		if((str[0] == '"' && str[str_end - 1] == '"')
		|| (str[0] == '\'' && str[str_end - 1] == '\'')
		|| (str[0] == '`' && str[str_end - 1] == '`')){
			str_start += 1;
			str_end -= 1;
		}
	}

	return string_copy_n(dest, dest_capacity,
			&str[str_start], (str_end - str_start));
}

void parse_motd(char *dest, int dest_capacity, const char *str){
	char motd_buf[256];
	parse_string(motd_buf, (int)sizeof(motd_buf), str);
	if(motd_buf[0] != 0){
		string_format(dest, dest_capacity, "%u\n%s", string_hash(motd_buf), motd_buf);
	}
}

bool read_config(const char *filename, ServerConfig *config){
	FILE *file = fopen(filename, "rb");
	if(file == NULL){
		LOG_ERR("Failed to open config file \"%s\"", filename);
		return false;
	}

	bool end_of_file = false;
	for(int line_number = 1; !end_of_file; line_number += 1){
		const int max_line_size = 1024;
		char line[max_line_size];
		int line_size = 0;
		int key_start = -1;
		int equal_pos = -1;
		while(true){
			int ch = fgetc(file);
			if(ch == EOF || ch == '\n'){
				if(ch == EOF){
					end_of_file = true;
				}
				break;
			}

			if(line_size < max_line_size){
				line[line_size] = (char)ch;
			}

			if(key_start == -1 && !isspace(ch)){
				key_start = line_size;
			}

			if(equal_pos == -1 && ch == '='){
				equal_pos = line_size;
			}

			line_size += 1;
		}

		// NOTE(fusion): Check line size limit.
		if(line_size > max_line_size){
			LOG_WARN("%s:%d: Exceeded line size limit of %d characters",
					filename, line_number, max_line_size);
			continue;
		}

		// NOTE(fusion): Check empty line or comment.
		if(key_start == -1 || line[key_start] == '#'){
			continue;
		}

		// NOTE(fusion): Check assignment.
		if(equal_pos == -1){
			LOG_WARN("%s:%d: No assignment found on non empty line",
					filename, line_number);
			continue;
		}

		// NOTE(fusion): Check empty key.
		int key_end = equal_pos;
		while(key_end > key_start && isspace(line[key_end - 1])){
			key_end -= 1;
		}

		if(key_start == key_end){
			LOG_WARN("%s:%d: Empty key", filename, line_number);
			continue;
		}

		// NOTE(fusion): Check empty value.
		int val_start = equal_pos + 1;
		int val_end = line_size;
		while(val_start < val_end && isspace(line[val_start])){
			val_start += 1;
		}

		while(val_end > val_start && isspace(line[val_end - 1])){
			val_end -= 1;
		}

		if(val_start == val_end){
			LOG_WARN("%s:%d: Empty value", filename, line_number);
			continue;
		}

		// NOTE(fusion): Parse KV pair.
		char key[256];
		if(!string_buf_copy_n(key, &line[key_start], (key_end - key_start))){
			LOG_WARN("%s:%d: Exceeded key size limit of %d characters",
					filename, line_number, (int)(sizeof(key) - 1));
			continue;
		}

		char val[256];
		if(!string_buf_copy_n(val, &line[val_start], (val_end - val_start))){
			LOG_WARN("%s:%d: Exceeded value size limit of %d characters",
					filename, line_number, (int)(sizeof(val) - 1));
			continue;
		}

		if(string_equals_ignore_case(key, "LoginPort")){
			parse_integer(&config->login_port, val);
		}else if(string_equals_ignore_case(key, "ConnectionTimeout")){
			parse_duration(&config->connection_timeout, val);
		}else if(string_equals_ignore_case(key, "MaxConnections")){
			parse_integer(&config->max_connections, val);
		}else if(string_equals_ignore_case(key, "MaxStatusRecords")){
			parse_integer(&config->max_status_records, val);
		}else if(string_equals_ignore_case(key, "MinStatusInterval")){
			parse_duration(&config->min_status_interval, val);
		}else if(string_equals_ignore_case(key, "QueryManagerHost")){
			parse_string_buf(config->query_manager_host, val);
		}else if(string_equals_ignore_case(key, "QueryManagerPort")){
			parse_integer(&config->query_manager_port, val);
		}else if(string_equals_ignore_case(key, "QueryManagerPassword")){
			parse_string_buf(config->query_manager_password, val);
		}else if(string_equals_ignore_case(key, "StatusWorld")){
			parse_string_buf(config->status_world, val);
		}else if(string_equals_ignore_case(key, "URL")){
			parse_string_buf(config->url, val);
		}else if(string_equals_ignore_case(key, "Location")){
			parse_string_buf(config->location, val);
		}else if(string_equals_ignore_case(key, "ServerType")){
			parse_string_buf(config->server_type, val);
		}else if(string_equals_ignore_case(key, "ServerVersion")){
			parse_string_buf(config->server_version, val);
		}else if(string_equals_ignore_case(key, "ClientVersion")){
			parse_string_buf(config->client_version, val);
		}else if(string_equals_ignore_case(key, "MOTD")){
			parse_motd(config->motd, sizeof(config->motd), val);
		}else if(string_equals_ignore_case(key, "TransportMode")){
			if(string_equals_ignore_case(val, "tcp")){
				config->transport_mode = TRANSPORT_TCP;
			}else if(string_equals_ignore_case(val, "websocket")){
				config->transport_mode = TRANSPORT_WEBSOCKET;
			}else if(string_equals_ignore_case(val, "both")){
				config->transport_mode = TRANSPORT_BOTH;
			}else{
				LOG_WARN("%s:%d: Invalid TransportMode \"%s\" (expected tcp, websocket, or both)",
						filename, line_number, val);
			}
		}else if(string_equals_ignore_case(key, "WebSocketPort")){
			parse_integer(&config->websocket_port, val);
		}else if(string_equals_ignore_case(key, "WebSocketAddress")){
			parse_string(config->websocket_address, (int)sizeof(config->websocket_address), val);
		}else{
			LOG_WARN("Unknown config \"%s\"", key);
		}
	}

	fclose(file);
	return true;
}
