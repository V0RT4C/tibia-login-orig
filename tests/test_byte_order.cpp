#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "common/byte_order.h"

TEST_CASE("read_u8") {
    uint8 buf[] = {0x42};
    CHECK(read_u8(buf) == 0x42);
}

TEST_CASE("read/write u16 little-endian") {
    uint8 buf[2] = {};
    write_u16_le(buf, 0x0102);
    CHECK(buf[0] == 0x02);
    CHECK(buf[1] == 0x01);
    CHECK(read_u16_le(buf) == 0x0102);
}

TEST_CASE("read/write u16 big-endian") {
    uint8 buf[2] = {};
    write_u16_be(buf, 0x0102);
    CHECK(buf[0] == 0x01);
    CHECK(buf[1] == 0x02);
    CHECK(read_u16_be(buf) == 0x0102);
}

TEST_CASE("read/write u32 little-endian") {
    uint8 buf[4] = {};
    write_u32_le(buf, 0x01020304);
    CHECK(buf[0] == 0x04);
    CHECK(buf[1] == 0x03);
    CHECK(buf[2] == 0x02);
    CHECK(buf[3] == 0x01);
    CHECK(read_u32_le(buf) == 0x01020304);
}

TEST_CASE("read/write u32 big-endian") {
    uint8 buf[4] = {};
    write_u32_be(buf, 0x01020304);
    CHECK(buf[0] == 0x01);
    CHECK(buf[1] == 0x02);
    CHECK(buf[2] == 0x03);
    CHECK(buf[3] == 0x04);
    CHECK(read_u32_be(buf) == 0x01020304);
}

TEST_CASE("read/write u64 little-endian") {
    uint8 buf[8] = {};
    write_u64_le(buf, 0x0102030405060708ULL);
    CHECK(buf[0] == 0x08);
    CHECK(buf[7] == 0x01);
    CHECK(read_u64_le(buf) == 0x0102030405060708ULL);
}

TEST_CASE("read/write u64 big-endian") {
    uint8 buf[8] = {};
    write_u64_be(buf, 0x0102030405060708ULL);
    CHECK(buf[0] == 0x01);
    CHECK(buf[7] == 0x08);
    CHECK(read_u64_be(buf) == 0x0102030405060708ULL);
}

TEST_CASE("roundtrip preserves zero") {
    uint8 buf[8] = {};
    write_u16_le(buf, 0);
    CHECK(read_u16_le(buf) == 0);
    write_u32_le(buf, 0);
    CHECK(read_u32_le(buf) == 0);
    write_u64_le(buf, 0);
    CHECK(read_u64_le(buf) == 0);
}

TEST_CASE("roundtrip preserves max values") {
    uint8 buf[8] = {};
    write_u16_le(buf, 0xFFFF);
    CHECK(read_u16_le(buf) == 0xFFFF);
    write_u32_le(buf, 0xFFFFFFFF);
    CHECK(read_u32_le(buf) == 0xFFFFFFFF);
    write_u64_le(buf, 0xFFFFFFFFFFFFFFFFULL);
    CHECK(read_u64_le(buf) == 0xFFFFFFFFFFFFFFFFULL);
}
