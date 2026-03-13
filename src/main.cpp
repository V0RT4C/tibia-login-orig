#include "common.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int          g_ShutdownSignal = 0;
ServerConfig g_config         = {};
QueryClient* query_client     = nullptr;

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

static bool init_query() {
	ASSERT(query_client == nullptr);
	query_client = new QueryClient();
	if (!query_client->connect(g_config.query_manager_host,
			g_config.query_manager_port, g_config.query_manager_password)) {
		LOG_ERR("Failed to connect to query manager");
		return false;
	}
	return true;
}

static void exit_query() {
	if (query_client != nullptr) {
		delete query_client;
		query_client = nullptr;
	}
}

int main(int argc, const char **argv){
	(void)argc;
	(void)argv;

	init_monotonic_clock();
	g_ShutdownSignal = 0;
	if(!SigHandler(SIGPIPE, SIG_IGN)
	|| !SigHandler(SIGINT, ShutdownHandler)
	|| !SigHandler(SIGTERM, ShutdownHandler)){
		return EXIT_FAILURE;
	}

	// Service Config
	g_config.login_port         = 7171;
	g_config.connection_timeout = 5;   // seconds
	g_config.max_connections    = 10;
	g_config.max_status_records  = 1024;
	g_config.min_status_interval = 300; // seconds
	string_buf_copy(g_config.query_manager_host, "127.0.0.1");
	g_config.query_manager_port  = 7173;
	string_buf_copy(g_config.query_manager_password, "");

	// Service Info
	string_buf_copy(g_config.status_world,   "");
	string_buf_copy(g_config.url,           "");
	string_buf_copy(g_config.location,      "");
	string_buf_copy(g_config.server_type,    "");
	string_buf_copy(g_config.server_version, "");
	string_buf_copy(g_config.client_version, "");
	string_buf_copy(g_config.motd,          "");

	g_config.transport_mode    = TRANSPORT_TCP;
	g_config.websocket_port    = 7172;
	string_buf_copy(g_config.websocket_address, "0.0.0.0");

	LOG("Tibia Login v0.2");
	if(!read_config("config.cfg", &g_config)){
		return EXIT_FAILURE;
	}

	LOG("Login port:          %d",     g_config.login_port);
	LOG("Connection timeout:  %ds",    g_config.connection_timeout);
	LOG("Max connections:     %d",     g_config.max_connections);
	LOG("Max status records:  %d",     g_config.max_status_records);
	LOG("Min status interval: %ds",    g_config.min_status_interval);
	LOG("Query manager host:  \"%s\"", g_config.query_manager_host);
	LOG("Query manager port:  %d",     g_config.query_manager_port);
	LOG("Status world:        \"%s\"", g_config.status_world);
	LOG("URL:                 \"%s\"", g_config.url);
	LOG("Location:            \"%s\"", g_config.location);
	LOG("Server type:         \"%s\"", g_config.server_type);
	LOG("Server version:      \"%s\"", g_config.server_version);
	LOG("Client version:      \"%s\"", g_config.client_version);

	{	// NOTE(fusion): Print MOTD preview with escape codes.
		char MotdPreview[30];
		if(string_buf_escape(MotdPreview, g_config.motd)){
			LOG("MOTD:                \"%s\"", MotdPreview);
		}else{
			LOG("MOTD:                \"%s...\"", MotdPreview);
		}
	}

	if(g_config.transport_mode == TRANSPORT_WEBSOCKET
	|| g_config.transport_mode == TRANSPORT_BOTH){
		LOG("WebSocket port:      %d",     g_config.websocket_port);
		LOG("WebSocket address:   \"%s\"", g_config.websocket_address);
	}

	atexit(exit_query);
	atexit(exit_connections);
	if(!init_query() || !init_connections()){
		return EXIT_FAILURE;
	}

	LOG("Running...");
	while(g_ShutdownSignal == 0){
		process_connections();
	}

	LOG("Received signal %d (%s), shutting down...",
			g_ShutdownSignal, strsignal(g_ShutdownSignal));
	return EXIT_SUCCESS;
}
