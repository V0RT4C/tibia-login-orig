#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "../src/common/types.h"
#include "../src/common/assert.h"
#include "../src/common/byte_order.h"
#include "../src/common/buffer_reader.h"
#include "../src/common/buffer_writer.h"
#include "../src/common/string_utils.h"
#include "../src/network/connection_types.h"
#include "../src/network/server.h"
#include "../src/crypto/rsa.h"
#include "../src/crypto/xtea.h"
#include "../src/query/query_types.h"
#include "../src/query/query_client.h"
#include "../src/query/login_query.h"
#include "../src/config/config.h"
#include "../src/login/login_handler.h"

#include <algorithm>
#include <cstring>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

// ---------------------------------------------------------------------------
// Test control globals
// ---------------------------------------------------------------------------
static bool g_close_called = false;
static int g_stub_login_result = 0;
static int g_stub_num_characters = 0;
static CharacterLoginData g_stub_characters[50] = {};
static int g_stub_premium_days = 0;

// ---------------------------------------------------------------------------
// Stubs for functions defined in server.cpp and login_query.cpp
// ---------------------------------------------------------------------------
void close_connection(Connection* connection) {
    g_close_called = true;
    connection->state = ConnectionState::Free;
}

int login_account(QueryClient& /*client*/, int /*account_id*/,
        const char* /*password*/, const char* /*ip_address*/,
        int max_characters, int* num_characters,
        CharacterLoginData* characters, int* premium_days) {
    *num_characters = std::min(g_stub_num_characters, max_characters);
    for (int i = 0; i < *num_characters; i++) {
        characters[i] = g_stub_characters[i];
    }
    *premium_days = g_stub_premium_days;
    return g_stub_login_result;
}

// ---------------------------------------------------------------------------
// RSA key management (generated once, shared across tests)
// ---------------------------------------------------------------------------
static const char* TEST_PEM = "/tmp/test_login_handler_key.pem";
static RSA* g_rsa = nullptr;
static std::unique_ptr<RsaKey> g_rsa_key;

static void ensure_rsa_key() {
    if (g_rsa) return;
    g_rsa = RSA_new();
    BIGNUM* e = BN_new();
    BN_set_word(e, RSA_F4);
    RSA_generate_key_ex(g_rsa, 1024, e, nullptr);
    BN_free(e);

    FILE* f = fopen(TEST_PEM, "wb");
    PEM_write_RSAPrivateKey(f, g_rsa, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(f);

    g_rsa_key = RsaKey::from_pem_file(TEST_PEM);
    std::remove(TEST_PEM);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static const uint32 TEST_XTEA[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};

static void reset_stubs() {
    ensure_rsa_key();
    g_close_called = false;
    g_stub_login_result = 0;
    g_stub_num_characters = 0;
    std::memset(g_stub_characters, 0, sizeof(g_stub_characters));
    g_stub_premium_days = 0;
}

static Connection make_connection() {
    Connection conn = {};
    conn.state = ConnectionState::Processing;
    conn.ip_address = (10 << 24) | (0 << 16) | (0 << 8) | 1; // 10.0.0.1
    string_buf_copy(conn.remote_address, "10.0.0.1");
    return conn;
}

// Build a valid 145-byte login packet with RSA-encrypted asymmetric data.
// terminal_version: protocol version (770 for default, 772 for TIBIA772 builds)
// account_id: the account number to log in with
// password: the account password
static void build_login_packet(Connection* conn, int account_id,
        const char* password, uint16 terminal_version = 770) {
    ensure_rsa_key();

    // Build asymmetric plaintext (128 bytes)
    uint8 plain[128] = {};
    BufferWriter asym(plain, sizeof(plain));
    asym.write_u8(0); // must be zero
    asym.write_u32(TEST_XTEA[0]);
    asym.write_u32(TEST_XTEA[1]);
    asym.write_u32(TEST_XTEA[2]);
    asym.write_u32(TEST_XTEA[3]);
    asym.write_u32(static_cast<uint32>(account_id));
    asym.write_string(password);

    // Encrypt with public key
    uint8 encrypted[128];
    RSA_public_encrypt(128, plain, encrypted, g_rsa, RSA_NO_PADDING);

    // Build the full 145-byte packet
    BufferWriter pkt(conn->buffer, sizeof(conn->buffer));
    pkt.write_u8(1);                // login request type
    pkt.write_u16(0);               // OS
    pkt.write_u16(terminal_version); // protocol version
    pkt.write_u32(0);               // DAT signature
    pkt.write_u32(0);               // SPR signature
    pkt.write_u32(0);               // PIC signature
    for (int i = 0; i < 128; i++) {
        pkt.write_u8(encrypted[i]);
    }

    conn->rw_size = 145;
}

// Decrypt the XTEA-encrypted response in the connection buffer and return
// a BufferReader positioned at the start of the response payload.
static BufferReader decrypt_response(Connection* conn) {
    // Response layout: [encrypted_size:u16] [XTEA-encrypted: data_size:u16, payload...]
    int encrypted_size = read_u16_le(conn->buffer);
    xtea_decrypt(TEST_XTEA, conn->buffer + 2, encrypted_size);
    int data_size = read_u16_le(conn->buffer + 2);
    return BufferReader(conn->buffer + 4, data_size);
}

// Read a string from a BufferReader into a local buffer and return it.
static std::string read_response_string(BufferReader& reader) {
    char buf[4096];
    reader.read_string(buf, sizeof(buf));
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("invalid packet size closes connection") {
    reset_stubs();
    Connection conn = make_connection();
    conn.rw_size = 100; // wrong size, should be 145

    ServerConfig config = {};
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK(g_close_called);
}

TEST_CASE("RSA decryption failure closes connection") {
    reset_stubs();
    Connection conn = make_connection();

    // Fill buffer with garbage (won't decrypt correctly)
    std::memset(conn.buffer, 0xAB, sizeof(conn.buffer));
    conn.rw_size = 145;

    ServerConfig config = {};
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK(g_close_called);
}

TEST_CASE("account ID zero sends error") {
    reset_stubs();
    Connection conn = make_connection();
    build_login_packet(&conn, 0, "secret");

    ServerConfig config = {};
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK_FALSE(g_close_called);
    CHECK(conn.state == ConnectionState::Writing);

    BufferReader resp = decrypt_response(&conn);
    CHECK(resp.read_u8() == 10); // LOGIN_ERROR
    std::string msg = read_response_string(resp);
    CHECK(msg == "You must enter an account number.");
}

TEST_CASE("wrong terminal version sends error") {
    reset_stubs();
    Connection conn = make_connection();
    build_login_packet(&conn, 12345, "secret", 750); // wrong version

    ServerConfig config = {};
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK_FALSE(g_close_called);
    CHECK(conn.state == ConnectionState::Writing);

    BufferReader resp = decrypt_response(&conn);
    CHECK(resp.read_u8() == 10); // LOGIN_ERROR
    std::string msg = read_response_string(resp);
    CHECK(msg.find("terminal version") != std::string::npos);
}

TEST_CASE("successful login returns character list") {
    reset_stubs();
    g_stub_login_result = 0;
    g_stub_num_characters = 2;
    g_stub_premium_days = 30;
    string_buf_copy(g_stub_characters[0].name, "Knight");
    string_buf_copy(g_stub_characters[0].world_name, "Senja");
    g_stub_characters[0].world_address = (127 << 24) | 1;
    g_stub_characters[0].world_port = 7172;
    string_buf_copy(g_stub_characters[1].name, "Sorcerer");
    string_buf_copy(g_stub_characters[1].world_name, "Senja");
    g_stub_characters[1].world_address = (127 << 24) | 1;
    g_stub_characters[1].world_port = 7172;

    Connection conn = make_connection();
    build_login_packet(&conn, 12345, "secret");

    ServerConfig config = {};
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK_FALSE(g_close_called);
    CHECK(conn.state == ConnectionState::Writing);

    BufferReader resp = decrypt_response(&conn);
    // No MOTD (config.motd is empty)
    CHECK(resp.read_u8() == 100); // CHARACTER_LIST
    CHECK(resp.read_u8() == 2);   // num_characters

    std::string name1 = read_response_string(resp);
    CHECK(name1 == "Knight");
    std::string world1 = read_response_string(resp);
    CHECK(world1 == "Senja");
    resp.read_u32_be(); // world_address
    resp.read_u16();    // world_port

    std::string name2 = read_response_string(resp);
    CHECK(name2 == "Sorcerer");
    std::string world2 = read_response_string(resp);
    CHECK(world2 == "Senja");
    resp.read_u32_be();
    resp.read_u16();

    CHECK(resp.read_u16() == 30); // premium_days
}

TEST_CASE("MOTD is included when configured") {
    reset_stubs();
    g_stub_login_result = 0;
    g_stub_num_characters = 1;
    g_stub_premium_days = 0;
    string_buf_copy(g_stub_characters[0].name, "Hero");
    string_buf_copy(g_stub_characters[0].world_name, "Senja");
    g_stub_characters[0].world_address = (127 << 24) | 1;
    g_stub_characters[0].world_port = 7172;

    Connection conn = make_connection();
    build_login_packet(&conn, 12345, "pass");

    ServerConfig config = {};
    string_buf_copy(config.motd, "1\nWelcome to Senja!");
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK(conn.state == ConnectionState::Writing);

    BufferReader resp = decrypt_response(&conn);
    CHECK(resp.read_u8() == 20); // MOTD
    std::string motd = read_response_string(resp);
    CHECK(motd == "1\nWelcome to Senja!");
    CHECK(resp.read_u8() == 100); // CHARACTER_LIST
}

TEST_CASE("login error codes produce correct messages") {
    struct ErrorCase {
        int code;
        const char* expected_message;
    };

    ErrorCase cases[] = {
        {1, "Accountnumber or password is not correct."},
        {2, "Accountnumber or password is not correct."},
        {3, "Account disabled for five minutes. Please wait."},
        {4, "IP address blocked for 30 minutes. Please wait."},
        {5, "Your account is banished."},
        {6, "Your IP address is banished."},
    };

    for (const auto& tc : cases) {
        CAPTURE(tc.code);
        reset_stubs();
        g_stub_login_result = tc.code;

        Connection conn = make_connection();
        build_login_packet(&conn, 12345, "secret");

        ServerConfig config = {};
        QueryClient client;
        process_login_request(&conn, *g_rsa_key, client, config);

        CHECK_FALSE(g_close_called);
        CHECK(conn.state == ConnectionState::Writing);

        BufferReader resp = decrypt_response(&conn);
        CHECK(resp.read_u8() == 10); // LOGIN_ERROR
        std::string msg = read_response_string(resp);
        CHECK(msg == tc.expected_message);
    }
}

TEST_CASE("unknown login error code sends internal error") {
    reset_stubs();
    g_stub_login_result = 99;

    Connection conn = make_connection();
    build_login_packet(&conn, 12345, "secret");

    ServerConfig config = {};
    QueryClient client;
    process_login_request(&conn, *g_rsa_key, client, config);

    CHECK_FALSE(g_close_called);
    CHECK(conn.state == ConnectionState::Writing);

    BufferReader resp = decrypt_response(&conn);
    CHECK(resp.read_u8() == 10);
    std::string msg = read_response_string(resp);
    CHECK(msg == "Internal error, closing connection.");
}

#pragma GCC diagnostic pop
