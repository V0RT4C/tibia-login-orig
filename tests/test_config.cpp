#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/config/config.h"

#include <cstring>
#include <cstdio>

TEST_CASE("parse_boolean") {
    bool val;
    CHECK(parse_boolean(&val, "true") == true);  CHECK(val == true);
    CHECK(parse_boolean(&val, "TRUE") == true);  CHECK(val == true);
    CHECK(parse_boolean(&val, "on") == true);    CHECK(val == true);
    CHECK(parse_boolean(&val, "yes") == true);   CHECK(val == true);
    CHECK(parse_boolean(&val, "false") == true);  CHECK(val == false);
    CHECK(parse_boolean(&val, "off") == true);    CHECK(val == false);
    CHECK(parse_boolean(&val, "no") == true);     CHECK(val == false);
    CHECK(parse_boolean(&val, "invalid") == false);
}

TEST_CASE("parse_integer") {
    int val;
    CHECK(parse_integer(&val, "42") == true);   CHECK(val == 42);
    CHECK(parse_integer(&val, "-1") == true);   CHECK(val == -1);
    CHECK(parse_integer(&val, "0") == true);    CHECK(val == 0);
    CHECK(parse_integer(&val, "0xFF") == true); CHECK(val == 255);
}

TEST_CASE("parse_duration") {
    int val;
    CHECK(parse_duration(&val, "5s") == true);  CHECK(val == 5);
    CHECK(parse_duration(&val, "5m") == true);  CHECK(val == 300);
    CHECK(parse_duration(&val, "2h") == true);  CHECK(val == 7200);
    CHECK(parse_duration(&val, "10") == true);  CHECK(val == 10);
}

TEST_CASE("parse_size") {
    int val;
    CHECK(parse_size(&val, "4k") == true);  CHECK(val == 4096);
    CHECK(parse_size(&val, "1M") == true);  CHECK(val == 1048576);
    CHECK(parse_size(&val, "512") == true); CHECK(val == 512);
}

TEST_CASE("parse_string - strips quotes") {
    char dest[32];
    CHECK(parse_string(dest, sizeof(dest), "\"Hello\"") == true);
    CHECK(std::strcmp(dest, "Hello") == 0);

    CHECK(parse_string(dest, sizeof(dest), "'World'") == true);
    CHECK(std::strcmp(dest, "World") == 0);

    CHECK(parse_string(dest, sizeof(dest), "NoQuotes") == true);
    CHECK(std::strcmp(dest, "NoQuotes") == 0);
}

TEST_CASE("read_config - from temp file") {
    const char* tmpfile = "/tmp/test_loginserver_config.cfg";
    FILE* f = std::fopen(tmpfile, "w");
    REQUIRE(f != nullptr);
    std::fprintf(f, "LoginPort = 7171\n");
    std::fprintf(f, "ConnectionTimeout = 10s\n");
    std::fprintf(f, "MaxConnections = 20\n");
    std::fprintf(f, "QueryManagerHost = \"192.168.1.1\"\n");
    std::fprintf(f, "QueryManagerPort = 7173\n");
    std::fprintf(f, "# This is a comment\n");
    std::fprintf(f, "ServerType = \"Tibia\"\n");
    std::fclose(f);

    ServerConfig config = {};
    CHECK(read_config(tmpfile, &config) == true);
    CHECK(config.login_port == 7171);
    CHECK(config.connection_timeout == 10);
    CHECK(config.max_connections == 20);
    CHECK(std::strcmp(config.query_manager_host, "192.168.1.1") == 0);
    CHECK(config.query_manager_port == 7173);
    CHECK(std::strcmp(config.server_type, "Tibia") == 0);

    std::remove(tmpfile);
}

TEST_CASE("parse TransportMode") {
    ServerConfig config = {};
    const char *path = "/tmp/test_transport_mode.cfg";

    SUBCASE("tcp") {
        FILE *f = fopen(path, "w");
        fprintf(f, "TransportMode = tcp\n");
        fclose(f);
        read_config(path, &config);
        CHECK(config.transport_mode == TRANSPORT_TCP);
    }
    SUBCASE("websocket") {
        FILE *f = fopen(path, "w");
        fprintf(f, "TransportMode = websocket\n");
        fclose(f);
        read_config(path, &config);
        CHECK(config.transport_mode == TRANSPORT_WEBSOCKET);
    }
    SUBCASE("both") {
        FILE *f = fopen(path, "w");
        fprintf(f, "TransportMode = both\n");
        fclose(f);
        read_config(path, &config);
        CHECK(config.transport_mode == TRANSPORT_BOTH);
    }

    remove(path);
}

TEST_CASE("parse WebSocketPort") {
    ServerConfig config = {};
    const char *path = "/tmp/test_ws_port.cfg";
    FILE *f = fopen(path, "w");
    fprintf(f, "WebSocketPort = 8080\n");
    fclose(f);
    read_config(path, &config);
    CHECK(config.websocket_port == 8080);
    remove(path);
}

TEST_CASE("parse WebSocketAddress") {
    ServerConfig config = {};
    const char *path = "/tmp/test_ws_addr.cfg";
    FILE *f = fopen(path, "w");
    fprintf(f, "WebSocketAddress = \"192.168.1.1\"\n");
    fclose(f);
    read_config(path, &config);
    CHECK(strcmp(config.websocket_address, "192.168.1.1") == 0);
    remove(path);
}
