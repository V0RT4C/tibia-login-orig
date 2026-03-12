#ifndef LOGINSERVER_COMMON_BUFFER_WRITER_H
#define LOGINSERVER_COMMON_BUFFER_WRITER_H

#include "types.h"

class BufferWriter {
public:
    uint8* buffer;
    int size;
    int position;

    BufferWriter();
    BufferWriter(uint8* buffer, int size);

    bool can_write(int bytes) const;
    bool overflowed() const;

    void write_flag(bool value);
    void write_u8(uint8 value);
    void write_u16(uint16 value);
    void write_u16_be(uint16 value);
    void write_u32(uint32 value);
    void write_u32_be(uint32 value);
    void write_string(const char* str);

    void rewrite_u16(int offset, uint16 value);
    void insert_u32(int offset, uint32 value);
};

#endif // LOGINSERVER_COMMON_BUFFER_WRITER_H
