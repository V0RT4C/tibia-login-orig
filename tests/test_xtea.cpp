#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/crypto/xtea.h"

#include <cstring>

TEST_CASE("xtea encrypt-decrypt roundtrip") {
    uint32 key[4] = {0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210};
    uint8 original[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8 data[16];
    std::memcpy(data, original, sizeof(data));

    xtea_encrypt(key, data, sizeof(data));
    CHECK(std::memcmp(data, original, sizeof(data)) != 0);

    xtea_decrypt(key, data, sizeof(data));
    CHECK(std::memcmp(data, original, sizeof(data)) == 0);
}

TEST_CASE("xtea with all-zero key and data") {
    uint32 key[4] = {0, 0, 0, 0};
    uint8 data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    xtea_encrypt(key, data, sizeof(data));
    xtea_decrypt(key, data, sizeof(data));

    uint8 expected[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    CHECK(std::memcmp(data, expected, sizeof(data)) == 0);
}

TEST_CASE("xtea processes only complete 8-byte blocks") {
    uint32 key[4] = {1, 2, 3, 4};
    uint8 data[12] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint8 original_tail[4] = {0x33, 0x44, 0x55, 0x66};

    xtea_encrypt(key, data, 12);
    CHECK(std::memcmp(data + 8, original_tail, 4) == 0);
}

TEST_CASE("xtea different keys produce different ciphertext") {
    uint32 key1[4] = {1, 2, 3, 4};
    uint32 key2[4] = {5, 6, 7, 8};
    uint8 data1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8 data2[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    xtea_encrypt(key1, data1, 8);
    xtea_encrypt(key2, data2, 8);
    CHECK(std::memcmp(data1, data2, 8) != 0);
}
