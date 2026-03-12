#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "common/buffer_reader.h"
#include "common/byte_order.h"

#include <cstring>

TEST_CASE("BufferReader default constructor") {
    BufferReader reader;
    CHECK(reader.buffer == nullptr);
    CHECK(reader.size == 0);
    CHECK(reader.position == 0);
}

TEST_CASE("read_u8") {
    uint8 data[] = {0x42, 0xFF};
    BufferReader reader(data, sizeof(data));
    CHECK(reader.read_u8() == 0x42);
    CHECK(reader.read_u8() == 0xFF);
    CHECK(reader.position == 2);
}

TEST_CASE("read_u16 little-endian") {
    uint8 data[2];
    write_u16_le(data, 0x1234);
    BufferReader reader(data, sizeof(data));
    CHECK(reader.read_u16() == 0x1234);
}

TEST_CASE("read_u32 little-endian") {
    uint8 data[4];
    write_u32_le(data, 0xDEADBEEF);
    BufferReader reader(data, sizeof(data));
    CHECK(reader.read_u32() == 0xDEADBEEF);
}

TEST_CASE("read_u32_be") {
    uint8 data[4];
    write_u32_be(data, 0xDEADBEEF);
    BufferReader reader(data, sizeof(data));
    CHECK(reader.read_u32_be() == 0xDEADBEEF);
}

TEST_CASE("read_flag") {
    uint8 data[] = {0x01, 0x00};
    BufferReader reader(data, sizeof(data));
    CHECK(reader.read_flag() == true);
    CHECK(reader.read_flag() == false);
}

TEST_CASE("read_bytes") {
    uint8 data[] = {1, 2, 3, 4, 5};
    BufferReader reader(data, sizeof(data));
    uint8 dest[3];
    reader.read_bytes(dest, 3);
    CHECK(dest[0] == 1);
    CHECK(dest[1] == 2);
    CHECK(dest[2] == 3);
    CHECK(reader.position == 3);
}

TEST_CASE("overflow detection") {
    uint8 data[2] = {0, 0};
    BufferReader reader(data, sizeof(data));
    reader.read_u32(); // reads past end
    CHECK(reader.overflowed() == true);
}

TEST_CASE("can_read boundary") {
    uint8 data[4] = {};
    BufferReader reader(data, sizeof(data));
    CHECK(reader.can_read(4) == true);
    CHECK(reader.can_read(5) == false);
    reader.read_u16();
    CHECK(reader.can_read(2) == true);
    CHECK(reader.can_read(3) == false);
}

TEST_CASE("read_string - short string") {
    // Build a buffer: u16 length (5) + "Hello"
    uint8 data[16] = {};
    write_u16_le(data, 5);
    std::memcpy(data + 2, "Hello", 5);

    BufferReader reader(data, sizeof(data));
    char dest[32] = {};
    reader.read_string(dest, sizeof(dest));
    // In non-UTF8 mode, Latin1 ASCII maps 1:1 to UTF8
    CHECK(std::strcmp(dest, "Hello") == 0);
}
