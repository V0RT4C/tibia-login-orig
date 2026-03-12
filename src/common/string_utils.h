#ifndef LOGINSERVER_COMMON_STRING_UTILS_H
#define LOGINSERVER_COMMON_STRING_UTILS_H

#include "assert.h"
#include "types.h"

bool string_empty(const char* str);
bool string_equals(const char* a, const char* b);
bool string_equals_ignore_case(const char* a, const char* b);
bool string_copy(char* dest, int dest_capacity, const char* src);
bool string_copy_n(char* dest, int dest_capacity, const char* src, int src_length);
bool string_format(char* dest, int dest_capacity, const char* format, ...) ATTR_PRINTF(3, 4);
bool string_format_time(char* dest, int dest_capacity, const char* format, int timestamp);
void string_clear(char* dest, int dest_capacity);
uint32 string_hash(const char* str);
bool string_escape(char* dest, int dest_capacity, const char* src);

// Helper macros for fixed-size char arrays
#define string_buf_copy(dest, src)               string_copy(dest, sizeof(dest), src)
#define string_buf_copy_n(dest, src, src_length) string_copy_n(dest, sizeof(dest), src, src_length)
#define string_buf_format(dest, ...)             string_format(dest, sizeof(dest), __VA_ARGS__)
#define string_buf_format_time(dest, fmt, ts)    string_format_time(dest, sizeof(dest), fmt, ts)
#define string_buf_clear(dest)                   string_clear(dest, sizeof(dest))
#define string_buf_escape(dest, src)             string_escape(dest, sizeof(dest), src)

#endif // LOGINSERVER_COMMON_STRING_UTILS_H
