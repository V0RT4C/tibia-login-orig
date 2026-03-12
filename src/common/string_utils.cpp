#include "string_utils.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

bool string_empty(const char *str){
	return str[0] == 0;
}

bool string_equals(const char *a, const char *b){
	int index = 0;
	while(a[index] != 0 && a[index] == b[index]){
		index += 1;
	}
	return a[index] == b[index];
}

bool string_equals_ignore_case(const char *a, const char *b){
	int index = 0;
	while(a[index] != 0 && tolower(a[index]) == tolower(b[index])){
		index += 1;
	}
	return tolower(a[index]) == tolower(b[index]);
}

bool string_copy(char *dest, int dest_capacity, const char *src){
	int src_length = (src != NULL ? (int)strlen(src) : 0);
	return string_copy_n(dest, dest_capacity, src, src_length);
}

bool string_copy_n(char *dest, int dest_capacity, const char *src, int src_length){
	ASSERT(dest_capacity > 0);
	bool result = (src_length < dest_capacity);
	if(result && src_length > 0){
		memcpy(dest, src, src_length);
		dest[src_length] = 0;
	}else{
		dest[0] = 0;
	}
	return result;
}

bool string_format(char *dest, int dest_capacity, const char *format, ...){
	va_list ap;
	va_start(ap, format);
	int written = vsnprintf(dest, dest_capacity, format, ap);
	va_end(ap);
	return written >= 0 && written < dest_capacity;
}

bool string_format_time(char *dest, int dest_capacity, const char *format, int timestamp){
	time_t t = static_cast<time_t>(timestamp);
	struct tm tm;
	localtime_r(&t, &tm);
	int result = static_cast<int>(strftime(dest, dest_capacity, format, &tm));

	// NOTE(fusion): `strftime` will return ZERO if it's unable to fit the result
	// in the supplied buffer, which is annoying because ZERO may not represent a
	// failure if the result is an empty string.
	ASSERT(result >= 0 && result < dest_capacity);
	if(result == 0){
		memset(dest, 0, dest_capacity);
	}

	return result != 0;
}

void string_clear(char *dest, int dest_capacity){
	ASSERT(dest_capacity > 0);
	memset(dest, 0, dest_capacity);
}

uint32 string_hash(const char *str){
	// FNV1a 32-bits
	uint32 hash = 0x811C9DC5U;
	for(int i = 0; str[i] != 0; i += 1){
		hash ^= (uint32)str[i];
		hash *= 0x01000193U;
	}
	return hash;
}

bool string_escape(char *dest, int dest_capacity, const char *src){
	int write_pos = 0;
	for(int read_pos = 0; src[read_pos] != 0 && write_pos < dest_capacity; read_pos += 1){
		int escape_ch = -1;
		switch(src[read_pos]){
			case '\a': escape_ch = 'a'; break;
			case '\b': escape_ch = 'b'; break;
			case '\t': escape_ch = 't'; break;
			case '\n': escape_ch = 'n'; break;
			case '\v': escape_ch = 'v'; break;
			case '\f': escape_ch = 'f'; break;
			case '\r': escape_ch = 'r'; break;
			case '\"': escape_ch = '\"'; break;
			case '\'': escape_ch = '\''; break;
			case '\\': escape_ch = '\\'; break;
		}

		if(escape_ch != -1){
			if((write_pos + 1) <= dest_capacity){
				dest[write_pos] = '\\';
				write_pos += 1;
			}

			if((write_pos + 1) <= dest_capacity){
				dest[write_pos] = escape_ch;
				write_pos += 1;
			}
		}else{
			if((write_pos + 1) <= dest_capacity){
				dest[write_pos] = src[read_pos];
				write_pos += 1;
			}
		}
	}

	if(write_pos < dest_capacity){
		dest[write_pos] = 0;
		return true;
	}else{
		dest[dest_capacity - 1] = 0;
		return false;
	}
}
