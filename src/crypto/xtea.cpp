#include "xtea.h"
#include "../common/assert.h"
#include "../common/byte_order.h"

void xtea_encrypt(const uint32* key, uint8* data, int size) {
    ASSERT(key != nullptr);
    while (size >= 8) {
        uint32 sum = 0x00000000UL;
        uint32 delta = 0x9E3779B9UL;
        uint32 v0 = read_u32_le(&data[0]);
        uint32 v1 = read_u32_le(&data[4]);
        for (int i = 0; i < 32; i += 1) {
            v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
            sum += delta;
            v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        }
        write_u32_le(&data[0], v0);
        write_u32_le(&data[4], v1);
        data += 8;
        size -= 8;
    }
}

void xtea_decrypt(const uint32* key, uint8* data, int size) {
    ASSERT(key != nullptr);
    while (size >= 8) {
        uint32 sum = 0xC6EF3720UL;
        uint32 delta = 0x9E3779B9UL;
        uint32 v0 = read_u32_le(&data[0]);
        uint32 v1 = read_u32_le(&data[4]);
        for (int i = 0; i < 32; i += 1) {
            v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
            sum -= delta;
            v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        }
        write_u32_le(&data[0], v0);
        write_u32_le(&data[4], v1);
        data += 8;
        size -= 8;
    }
}
