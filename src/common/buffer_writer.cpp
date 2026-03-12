#include "buffer_writer.h"
#include "byte_order.h"
#include "utf8.h"
#include "assert.h"

#include <cstring>

BufferWriter::BufferWriter() : BufferWriter(nullptr, 0) {}
BufferWriter::BufferWriter(uint8* buffer, int size)
    : buffer(buffer), size(size), position(0) {}

bool BufferWriter::can_write(int bytes) const {
    return (position + bytes) <= size;
}

bool BufferWriter::overflowed() const {
    return position > size;
}

void BufferWriter::write_flag(bool value) {
    write_u8(value ? 0x01 : 0x00);
}

void BufferWriter::write_u8(uint8 value) {
    if (can_write(1)) {
        buffer[position] = value;
    }
    position += 1;
}

void BufferWriter::write_u16(uint16 value) {
    if (can_write(2)) {
        write_u16_le(buffer + position, value);
    }
    position += 2;
}

void BufferWriter::write_u16_be(uint16 value) {
    if (can_write(2)) {
        ::write_u16_be(buffer + position, value);
    }
    position += 2;
}

void BufferWriter::write_u32(uint32 value) {
    if (can_write(4)) {
        write_u32_le(buffer + position, value);
    }
    position += 4;
}

void BufferWriter::write_u32_be(uint32 value) {
    if (can_write(4)) {
        ::write_u32_be(buffer + position, value);
    }
    position += 4;
}

void BufferWriter::write_string(const char* str) {
    if constexpr (CLIENT_ENCODING_UTF8) {
        int string_length = 0;
        if (str != nullptr) {
            string_length = static_cast<int>(std::strlen(str));
        }

        if (string_length < 0xFFFF) {
            write_u16(static_cast<uint16>(string_length));
        } else {
            write_u16(0xFFFF);
            write_u32(static_cast<uint32>(string_length));
        }

        if (string_length > 0 && can_write(string_length)) {
            std::memcpy(buffer + position, str, string_length);
        }

        position += string_length;
    } else {
        int string_length = 0;
        int output_length = 0;
        if (str != nullptr) {
            string_length = static_cast<int>(std::strlen(str));
            output_length = utf8_to_latin1(nullptr, 0, str, string_length);
        }

        if (output_length < 0xFFFF) {
            write_u16(static_cast<uint16>(output_length));
        } else {
            write_u16(0xFFFF);
            write_u32(static_cast<uint32>(output_length));
        }

        if (output_length > 0 && can_write(output_length)) {
            int written = utf8_to_latin1(
                reinterpret_cast<char*>(buffer + position),
                size - position, str, string_length);
            ASSERT(written == output_length);
        }

        position += output_length;
    }
}

void BufferWriter::rewrite_u16(int offset, uint16 value) {
    if ((offset + 2) <= position && !overflowed()) {
        write_u16_le(buffer + offset, value);
    }
}

void BufferWriter::insert_u32(int offset, uint32 value) {
    if (offset <= position) {
        if (can_write(4)) {
            std::memmove(buffer + offset + 4,
                         buffer + offset,
                         position - offset);
            write_u32_le(buffer + offset, value);
        }
        position += 4;
    }
}
