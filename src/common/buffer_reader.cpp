#include "buffer_reader.h"
#include "byte_order.h"
#include "utf8.h"

#include <cstring>

BufferReader::BufferReader() : BufferReader(nullptr, 0) {}
BufferReader::BufferReader(uint8* buffer, int size)
    : buffer(buffer), size(size), position(0) {}

bool BufferReader::can_read(int bytes) const {
    return (position + bytes) <= size;
}

bool BufferReader::overflowed() const {
    return position > size;
}

bool BufferReader::read_flag() {
    return read_u8() != 0x00;
}

uint8 BufferReader::read_u8() {
    uint8 result = 0;
    if (can_read(1)) {
        result = buffer[position];
    }
    position += 1;
    return result;
}

uint16 BufferReader::read_u16() {
    uint16 result = 0;
    if (can_read(2)) {
        result = read_u16_le(buffer + position);
    }
    position += 2;
    return result;
}

uint16 BufferReader::read_u16_be() {
    uint16 result = 0;
    if (can_read(2)) {
        result = ::read_u16_be(buffer + position);
    }
    position += 2;
    return result;
}

uint32 BufferReader::read_u32() {
    uint32 result = 0;
    if (can_read(4)) {
        result = read_u32_le(buffer + position);
    }
    position += 4;
    return result;
}

uint32 BufferReader::read_u32_be() {
    uint32 result = 0;
    if (can_read(4)) {
        result = ::read_u32_be(buffer + position);
    }
    position += 4;
    return result;
}

void BufferReader::read_string(char* dest, int dest_capacity) {
    int length = static_cast<int>(read_u16());
    if (length == 0xFFFF) {
        length = static_cast<int>(read_u32());
    }

    if constexpr (CLIENT_ENCODING_UTF8) {
        if (dest != nullptr && dest_capacity > 0) {
            int written = 0;
            if (can_read(length) && length < dest_capacity) {
                std::memcpy(dest, buffer + position, length);
                written = length;
            }
            std::memset(dest + written, 0, dest_capacity - written);
        }
    } else {
        if (dest != nullptr && dest_capacity > 0) {
            int written = 0;
            if (can_read(length)) {
                const char* src = reinterpret_cast<const char*>(buffer + position);
                written = latin1_to_utf8(dest, dest_capacity, src, length);
                if (written >= dest_capacity) {
                    written = 0;
                }
            }
            std::memset(dest + written, 0, dest_capacity - written);
        }
    }

    position += length;
}

void BufferReader::read_bytes(uint8* dest, int count) {
    if (can_read(count)) {
        std::memcpy(dest, buffer + position, count);
    }
    position += count;
}
