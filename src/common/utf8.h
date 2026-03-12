#ifndef LOGINSERVER_COMMON_UTF8_H
#define LOGINSERVER_COMMON_UTF8_H

#include "types.h"

int utf8_sequence_size(uint8 leading_byte);
bool utf8_is_trailing_byte(uint8 byte);
int utf8_encoded_size(int codepoint);
int utf8_find_next_leading_byte(const char* src, int src_length);
int utf8_decode_one(const uint8* src, int src_length, int* out_codepoint);
int utf8_encode_one(uint8* dest, int dest_capacity, int codepoint);
int utf8_to_latin1(char* dest, int dest_capacity, const char* src, int src_length);
int latin1_to_utf8(char* dest, int dest_capacity, const char* src, int src_length);

#endif // LOGINSERVER_COMMON_UTF8_H
