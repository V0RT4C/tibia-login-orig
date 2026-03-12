#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "common/buffer_writer.h"
#include "common/buffer_reader.h"
#include "common/byte_order.h"

#include <cstring>

TEST_CASE("BufferWriter default constructor") {
    BufferWriter writer;
    CHECK(writer.buffer == nullptr);
    CHECK(writer.size == 0);
    CHECK(writer.position == 0);
}

TEST_CASE("write_u8") {
    uint8 buf[4] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_u8(0x42);
    CHECK(buf[0] == 0x42);
    CHECK(writer.position == 1);
}

TEST_CASE("write_u16") {
    uint8 buf[4] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_u16(0x1234);
    CHECK(read_u16_le(buf) == 0x1234);
    CHECK(writer.position == 2);
}

TEST_CASE("write_u32") {
    uint8 buf[4] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_u32(0xDEADBEEF);
    CHECK(read_u32_le(buf) == 0xDEADBEEF);
}

TEST_CASE("write_flag") {
    uint8 buf[2] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_flag(true);
    CHECK(buf[0] == 0x01);
    writer.write_flag(false);
    CHECK(buf[1] == 0x00);
}

TEST_CASE("overflow detection") {
    uint8 buf[2] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_u32(0x12345678); // overflows 2-byte buffer
    CHECK(writer.overflowed() == true);
}

TEST_CASE("rewrite_u16") {
    uint8 buf[8] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_u16(0x0000); // placeholder at position 0
    writer.write_u16(0xBBBB);
    writer.rewrite_u16(0, 0xAAAA); // overwrite the placeholder
    CHECK(read_u16_le(buf) == 0xAAAA);
    CHECK(read_u16_le(buf + 2) == 0xBBBB);
}

TEST_CASE("insert_u32") {
    uint8 buf[16] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_u8(0xAA);
    writer.write_u8(0xBB);
    writer.insert_u32(1, 0x12345678); // insert at position 1, shifting 0xBB
    CHECK(buf[0] == 0xAA);
    CHECK(read_u32_le(buf + 1) == 0x12345678);
    CHECK(buf[5] == 0xBB);
    CHECK(writer.position == 6);
}

TEST_CASE("write_string - short ASCII") {
    uint8 buf[32] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_string("Hello");
    // In non-UTF8 mode: length is the Latin1 output length
    // For ASCII, Latin1 length == UTF8 length == 5
    CHECK(read_u16_le(buf) == 5);
    CHECK(std::memcmp(buf + 2, "Hello", 5) == 0);
    CHECK(writer.position == 7);
}

TEST_CASE("write_string - null") {
    uint8 buf[32] = {};
    BufferWriter writer(buf, sizeof(buf));
    writer.write_string(nullptr);
    CHECK(read_u16_le(buf) == 0);
    CHECK(writer.position == 2);
}
