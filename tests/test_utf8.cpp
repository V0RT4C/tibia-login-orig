#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "common/utf8.h"

#include <cstring>

TEST_CASE("utf8_sequence_size") {
    CHECK(utf8_sequence_size(0x00) == 1);  // ASCII
    CHECK(utf8_sequence_size(0x7F) == 1);  // max ASCII
    CHECK(utf8_sequence_size(0xC0) == 2);  // 2-byte start
    CHECK(utf8_sequence_size(0xDF) == 2);
    CHECK(utf8_sequence_size(0xE0) == 3);  // 3-byte start
    CHECK(utf8_sequence_size(0xEF) == 3);
    CHECK(utf8_sequence_size(0xF0) == 4);  // 4-byte start
    CHECK(utf8_sequence_size(0xF7) == 4);
    CHECK(utf8_sequence_size(0x80) == 0);  // trailing byte (invalid lead)
    CHECK(utf8_sequence_size(0xFE) == 0);  // invalid
}

TEST_CASE("utf8_is_trailing_byte") {
    CHECK(utf8_is_trailing_byte(0x80) == true);
    CHECK(utf8_is_trailing_byte(0xBF) == true);
    CHECK(utf8_is_trailing_byte(0x7F) == false);
    CHECK(utf8_is_trailing_byte(0xC0) == false);
}

TEST_CASE("utf8_encoded_size") {
    CHECK(utf8_encoded_size(0x00) == 1);
    CHECK(utf8_encoded_size(0x7F) == 1);
    CHECK(utf8_encoded_size(0x80) == 2);
    CHECK(utf8_encoded_size(0x07FF) == 2);
    CHECK(utf8_encoded_size(0x0800) == 3);
    CHECK(utf8_encoded_size(0xFFFF) == 3);
    CHECK(utf8_encoded_size(0x10000) == 4);
    CHECK(utf8_encoded_size(0x10FFFF) == 4);
    CHECK(utf8_encoded_size(-1) == 0);
    CHECK(utf8_encoded_size(0x110000) == 0);
}

TEST_CASE("utf8 encode/decode roundtrip - ASCII") {
    uint8 buf[4];
    int written = utf8_encode_one(buf, sizeof(buf), 'A');
    CHECK(written == 1);
    CHECK(buf[0] == 'A');

    int codepoint = 0;
    int read = utf8_decode_one(buf, written, &codepoint);
    CHECK(read == 1);
    CHECK(codepoint == 'A');
}

TEST_CASE("utf8 encode/decode roundtrip - 2-byte") {
    uint8 buf[4];
    int written = utf8_encode_one(buf, sizeof(buf), 0xE9); // e-acute
    CHECK(written == 2);

    int codepoint = 0;
    int read = utf8_decode_one(buf, written, &codepoint);
    CHECK(read == 2);
    CHECK(codepoint == 0xE9);
}

TEST_CASE("utf8 encode/decode roundtrip - 3-byte") {
    uint8 buf[4];
    int written = utf8_encode_one(buf, sizeof(buf), 0x20AC); // euro sign
    CHECK(written == 3);

    int codepoint = 0;
    int read = utf8_decode_one(buf, written, &codepoint);
    CHECK(read == 3);
    CHECK(codepoint == 0x20AC);
}

TEST_CASE("utf8_to_latin1 - ASCII passthrough") {
    const char* src = "Hello";
    char dest[16];
    int written = utf8_to_latin1(dest, sizeof(dest), src, 5);
    CHECK(written == 5);
    CHECK(std::memcmp(dest, "Hello", 5) == 0);
}

TEST_CASE("utf8_to_latin1 - non-latin1 becomes ?") {
    // Euro sign U+20AC (3 bytes in UTF-8: E2 82 AC) -> '?'
    const char src[] = {(char)0xE2, (char)0x82, (char)0xAC, 0};
    char dest[4];
    int written = utf8_to_latin1(dest, sizeof(dest), src, 3);
    CHECK(written == 1);
    CHECK(dest[0] == '?');
}

TEST_CASE("latin1_to_utf8 - ASCII passthrough") {
    const char* src = "Hello";
    char dest[16];
    int written = latin1_to_utf8(dest, sizeof(dest), src, 5);
    CHECK(written == 5);
    CHECK(std::memcmp(dest, "Hello", 5) == 0);
}

TEST_CASE("latin1_to_utf8 - high byte encodes to 2 bytes") {
    // Latin1 0xE9 (e-acute) -> UTF-8 C3 A9
    char src[] = {(char)0xE9};
    char dest[4];
    int written = latin1_to_utf8(dest, sizeof(dest), src, 1);
    CHECK(written == 2);
    CHECK((uint8)dest[0] == 0xC3);
    CHECK((uint8)dest[1] == 0xA9);
}
