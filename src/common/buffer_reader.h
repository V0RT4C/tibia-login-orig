#ifndef LOGINSERVER_COMMON_BUFFER_READER_H
#define LOGINSERVER_COMMON_BUFFER_READER_H

#include "types.h"

class BufferReader {
public:
    uint8* buffer;
    int size;
    int position;

    BufferReader();
    BufferReader(uint8* buffer, int size);

    bool can_read(int bytes) const;
    bool overflowed() const;

    bool read_flag();
    uint8 read_u8();
    uint16 read_u16();
    uint16 read_u16_be();
    uint32 read_u32();
    uint32 read_u32_be();
    void read_string(char* dest, int dest_capacity);
    void read_bytes(uint8* dest, int count);
};

#endif // LOGINSERVER_COMMON_BUFFER_READER_H
