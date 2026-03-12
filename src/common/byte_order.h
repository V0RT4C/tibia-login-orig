#ifndef LOGINSERVER_COMMON_BYTE_ORDER_H
#define LOGINSERVER_COMMON_BYTE_ORDER_H

#include "types.h"

inline uint8 read_u8(const uint8* buffer) {
    return buffer[0];
}

inline uint16 read_u16_le(const uint8* buffer) {
    return static_cast<uint16>(buffer[0])
         | (static_cast<uint16>(buffer[1]) << 8);
}

inline uint16 read_u16_be(const uint8* buffer) {
    return (static_cast<uint16>(buffer[0]) << 8)
         | static_cast<uint16>(buffer[1]);
}

inline uint32 read_u32_le(const uint8* buffer) {
    return static_cast<uint32>(buffer[0])
         | (static_cast<uint32>(buffer[1]) << 8)
         | (static_cast<uint32>(buffer[2]) << 16)
         | (static_cast<uint32>(buffer[3]) << 24);
}

inline uint32 read_u32_be(const uint8* buffer) {
    return (static_cast<uint32>(buffer[0]) << 24)
         | (static_cast<uint32>(buffer[1]) << 16)
         | (static_cast<uint32>(buffer[2]) << 8)
         | static_cast<uint32>(buffer[3]);
}

inline uint64 read_u64_le(const uint8* buffer) {
    return static_cast<uint64>(buffer[0])
         | (static_cast<uint64>(buffer[1]) << 8)
         | (static_cast<uint64>(buffer[2]) << 16)
         | (static_cast<uint64>(buffer[3]) << 24)
         | (static_cast<uint64>(buffer[4]) << 32)
         | (static_cast<uint64>(buffer[5]) << 40)
         | (static_cast<uint64>(buffer[6]) << 48)
         | (static_cast<uint64>(buffer[7]) << 56);
}

inline uint64 read_u64_be(const uint8* buffer) {
    return (static_cast<uint64>(buffer[0]) << 56)
         | (static_cast<uint64>(buffer[1]) << 48)
         | (static_cast<uint64>(buffer[2]) << 40)
         | (static_cast<uint64>(buffer[3]) << 32)
         | (static_cast<uint64>(buffer[4]) << 24)
         | (static_cast<uint64>(buffer[5]) << 16)
         | (static_cast<uint64>(buffer[6]) << 8)
         | static_cast<uint64>(buffer[7]);
}

inline void write_u8(uint8* buffer, uint8 value) {
    buffer[0] = value;
}

inline void write_u16_le(uint8* buffer, uint16 value) {
    buffer[0] = static_cast<uint8>(value >> 0);
    buffer[1] = static_cast<uint8>(value >> 8);
}

inline void write_u16_be(uint8* buffer, uint16 value) {
    buffer[0] = static_cast<uint8>(value >> 8);
    buffer[1] = static_cast<uint8>(value >> 0);
}

inline void write_u32_le(uint8* buffer, uint32 value) {
    buffer[0] = static_cast<uint8>(value >>  0);
    buffer[1] = static_cast<uint8>(value >>  8);
    buffer[2] = static_cast<uint8>(value >> 16);
    buffer[3] = static_cast<uint8>(value >> 24);
}

inline void write_u32_be(uint8* buffer, uint32 value) {
    buffer[0] = static_cast<uint8>(value >> 24);
    buffer[1] = static_cast<uint8>(value >> 16);
    buffer[2] = static_cast<uint8>(value >>  8);
    buffer[3] = static_cast<uint8>(value >>  0);
}

inline void write_u64_le(uint8* buffer, uint64 value) {
    buffer[0] = static_cast<uint8>(value >>  0);
    buffer[1] = static_cast<uint8>(value >>  8);
    buffer[2] = static_cast<uint8>(value >> 16);
    buffer[3] = static_cast<uint8>(value >> 24);
    buffer[4] = static_cast<uint8>(value >> 32);
    buffer[5] = static_cast<uint8>(value >> 40);
    buffer[6] = static_cast<uint8>(value >> 48);
    buffer[7] = static_cast<uint8>(value >> 56);
}

inline void write_u64_be(uint8* buffer, uint64 value) {
    buffer[0] = static_cast<uint8>(value >> 56);
    buffer[1] = static_cast<uint8>(value >> 48);
    buffer[2] = static_cast<uint8>(value >> 40);
    buffer[3] = static_cast<uint8>(value >> 32);
    buffer[4] = static_cast<uint8>(value >> 24);
    buffer[5] = static_cast<uint8>(value >> 16);
    buffer[6] = static_cast<uint8>(value >>  8);
    buffer[7] = static_cast<uint8>(value >>  0);
}

#endif // LOGINSERVER_COMMON_BYTE_ORDER_H
