#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/config/config.h"

#include <cstdlib>
#include <cstring>

// Helper: set an env var for the duration of a test.
static void SetEnv(const char *name, const char *value){
	setenv(name, value, 1);
}
static void UnsetEnv(const char *name){
	unsetenv(name);
}

TEST_CASE("read_config - defaults when only required var set") {
	SetEnv("SENJA_QM_PASSWORD", "secret");

	ServerConfig config = {};
	CHECK(read_config(&config) == true);

	CHECK(config.login_port == 7171);
	CHECK(config.connection_timeout == 5);
	CHECK(config.max_connections == 10);
	CHECK(config.max_status_records == 1024);
	CHECK(config.min_status_interval == 300);
	CHECK(std::strcmp(config.query_manager_host, "127.0.0.1") == 0);
	CHECK(config.query_manager_port == 7173);
	CHECK(std::strcmp(config.query_manager_password, "secret") == 0);
	CHECK(std::strcmp(config.status_world, "Senja") == 0);
	CHECK(std::strcmp(config.server_type, "Tibia") == 0);
	CHECK(std::strcmp(config.server_version, "7.7") == 0);
	CHECK(std::strcmp(config.client_version, "7.7") == 0);
	CHECK(config.transport_mode == TRANSPORT_BOTH);
	CHECK(config.websocket_port == 9172);
	CHECK(std::strcmp(config.websocket_address, "0.0.0.0") == 0);

	UnsetEnv("SENJA_QM_PASSWORD");
}

TEST_CASE("read_config - fails without SENJA_QM_PASSWORD") {
	UnsetEnv("SENJA_QM_PASSWORD");
	ServerConfig config = {};
	CHECK(read_config(&config) == false);
}

TEST_CASE("read_config - overrides from env") {
	SetEnv("SENJA_QM_PASSWORD",     "mypass");
	SetEnv("SENJA_LOGIN_PORT",      "8888");
	SetEnv("SENJA_MAX_CONNECTIONS", "50");
	SetEnv("SENJA_QM_HOST",         "10.0.0.1");
	SetEnv("SENJA_QM_PORT",         "7174");
	SetEnv("SENJA_WORLD_NAME",      "TestWorld");

	ServerConfig config = {};
	CHECK(read_config(&config) == true);

	CHECK(config.login_port == 8888);
	CHECK(config.max_connections == 50);
	CHECK(std::strcmp(config.query_manager_host, "10.0.0.1") == 0);
	CHECK(config.query_manager_port == 7174);
	CHECK(std::strcmp(config.query_manager_password, "mypass") == 0);
	CHECK(std::strcmp(config.status_world, "TestWorld") == 0);

	UnsetEnv("SENJA_QM_PASSWORD");
	UnsetEnv("SENJA_LOGIN_PORT");
	UnsetEnv("SENJA_MAX_CONNECTIONS");
	UnsetEnv("SENJA_QM_HOST");
	UnsetEnv("SENJA_QM_PORT");
	UnsetEnv("SENJA_WORLD_NAME");
}

TEST_CASE("read_config - TransportMode tcp") {
	SetEnv("SENJA_QM_PASSWORD",     "x");
	SetEnv("SENJA_TRANSPORT_MODE",  "tcp");

	ServerConfig config = {};
	CHECK(read_config(&config) == true);
	CHECK(config.transport_mode == TRANSPORT_TCP);

	UnsetEnv("SENJA_QM_PASSWORD");
	UnsetEnv("SENJA_TRANSPORT_MODE");
}

TEST_CASE("read_config - TransportMode websocket") {
	SetEnv("SENJA_QM_PASSWORD",     "x");
	SetEnv("SENJA_TRANSPORT_MODE",  "websocket");

	ServerConfig config = {};
	CHECK(read_config(&config) == true);
	CHECK(config.transport_mode == TRANSPORT_WEBSOCKET);

	UnsetEnv("SENJA_QM_PASSWORD");
	UnsetEnv("SENJA_TRANSPORT_MODE");
}

TEST_CASE("read_config - TransportMode both (default)") {
	SetEnv("SENJA_QM_PASSWORD",     "x");
	SetEnv("SENJA_TRANSPORT_MODE",  "both");

	ServerConfig config = {};
	CHECK(read_config(&config) == true);
	CHECK(config.transport_mode == TRANSPORT_BOTH);

	UnsetEnv("SENJA_QM_PASSWORD");
	UnsetEnv("SENJA_TRANSPORT_MODE");
}

TEST_CASE("read_config - WebSocket port and address") {
	SetEnv("SENJA_QM_PASSWORD",       "x");
	SetEnv("SENJA_WEBSOCKET_PORT",    "8080");
	SetEnv("SENJA_WEBSOCKET_ADDRESS", "127.0.0.1");

	ServerConfig config = {};
	CHECK(read_config(&config) == true);
	CHECK(config.websocket_port == 8080);
	CHECK(std::strcmp(config.websocket_address, "127.0.0.1") == 0);

	UnsetEnv("SENJA_QM_PASSWORD");
	UnsetEnv("SENJA_WEBSOCKET_PORT");
	UnsetEnv("SENJA_WEBSOCKET_ADDRESS");
}

TEST_CASE("parse_motd - formats hash and text") {
	char dest[256] = {};
	parse_motd(dest, sizeof(dest), "Hello World");
	// Should be non-empty and contain a newline separating hash from text
	CHECK(dest[0] != '\0');
	const char *nl = std::strchr(dest, '\n');
	REQUIRE(nl != nullptr);
	CHECK(std::strcmp(nl + 1, "Hello World") == 0);
}
