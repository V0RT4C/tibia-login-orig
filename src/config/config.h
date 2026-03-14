#ifndef LOGINSERVER_CONFIG_CONFIG_H
#define LOGINSERVER_CONFIG_CONFIG_H

#include "../common/types.h"
#include "../common/assert.h"

enum TransportMode {
	TRANSPORT_TCP       = 0,
	TRANSPORT_WEBSOCKET = 1,
	TRANSPORT_BOTH      = 2,
};

struct ServerConfig {
    // Service Config
    int login_port;
    int connection_timeout;
    int max_connections;
    int max_status_records;
    int min_status_interval;
    char query_manager_host[100];
    int query_manager_port;
    char query_manager_password[30];

    // Service Info
    char status_world[30];
    char url[100];
    char location[30];
    char server_type[30];
    char server_version[30];
    char client_version[30];
    char motd[256];

    // WebSocket Config
    TransportMode transport_mode;
    int websocket_port;
    char websocket_address[16];
};

void parse_motd(char* dest, int dest_capacity, const char* str);
bool read_config(ServerConfig* config);

#endif // LOGINSERVER_CONFIG_CONFIG_H
