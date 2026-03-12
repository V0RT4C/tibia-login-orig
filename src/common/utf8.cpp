#include "utf8.h"

#include <cstring>

int utf8_sequence_size(uint8 leading_byte){
	if((leading_byte & 0x80) == 0){
		return 1;
	}else if((leading_byte & 0xE0) == 0xC0){
		return 2;
	}else if((leading_byte & 0xF0) == 0xE0){
		return 3;
	}else if((leading_byte & 0xF8) == 0xF0){
		return 4;
	}else{
		return 0;
	}
}

bool utf8_is_trailing_byte(uint8 byte){
	return (byte & 0xC0) == 0x80;
}

int utf8_encoded_size(int codepoint){
	if(codepoint < 0){
		return 0;
	}else if(codepoint <= 0x7F){
		return 1;
	}else if(codepoint <= 0x07FF){
		return 2;
	}else if(codepoint <= 0xFFFF){
		return 3;
	}else if(codepoint <= 0x10FFFF){
		return 4;
	}else{
		return 0;
	}
}

int utf8_find_next_leading_byte(const char* src, int src_length){
	int offset = 0;
	while(offset < src_length){
		// NOTE(fusion): Allow the first byte to be a leading byte, in case we
		// just want to advance from one leading byte to another.
		if(offset > 0 && !utf8_is_trailing_byte(src[offset])){
			break;
		}
		offset += 1;
	}
	return offset;
}

int utf8_decode_one(const uint8* src, int src_length, int* out_codepoint){
	if(src_length <= 0){
		return 0;
	}

	int size = utf8_sequence_size(src[0]);
	if(size <= 0 || size > src_length){
		return 0;
	}

	for(int i = 1; i < size; i += 1){
		if(!utf8_is_trailing_byte(src[i])){
			return 0;
		}
	}

	int codepoint = 0;
	switch(size){
		case 1:{
			codepoint = (int)src[0];
			break;
		}

		case 2:{
			codepoint = ((int)(src[0] & 0x1F) <<  6)
					|   ((int)(src[1] & 0x3F) <<  0);
			break;
		}

		case 3:{
			codepoint = ((int)(src[0] & 0x0F) << 12)
					|   ((int)(src[1] & 0x3F) <<  6)
					|   ((int)(src[2] & 0x3F) <<  0);
			break;
		}

		case 4:{
			codepoint = ((int)(src[0] & 0x07) << 18)
					|   ((int)(src[1] & 0x3F) << 12)
					|   ((int)(src[2] & 0x3F) <<  6)
					|   ((int)(src[3] & 0x3F) <<  0);
			break;
		}
	}

	if(out_codepoint){
		*out_codepoint = codepoint;
	}

	return size;
}

int utf8_encode_one(uint8* dest, int dest_capacity, int codepoint){
	int size = utf8_encoded_size(codepoint);
	if(size > 0 && size <= dest_capacity){
		switch(size){
			case 1:{
				dest[0] = (uint8)codepoint;
				break;
			}

			case 2:{
				dest[0] = (uint8)(0xC0 | (0x1F & (codepoint >>  6)));
				dest[1] = (uint8)(0x80 | (0x3F & (codepoint >>  0)));
				break;
			}

			case 3:{
				dest[0] = (uint8)(0xE0 | (0x0F & (codepoint >> 12)));
				dest[1] = (uint8)(0x80 | (0x3F & (codepoint >>  6)));
				dest[2] = (uint8)(0x80 | (0x3F & (codepoint >>  0)));
				break;
			}

			case 4:{
				dest[0] = (uint8)(0xF0 | (0x07 & (codepoint >> 18)));
				dest[1] = (uint8)(0x80 | (0x3F & (codepoint >> 12)));
				dest[2] = (uint8)(0x80 | (0x3F & (codepoint >>  6)));
				dest[3] = (uint8)(0x80 | (0x3F & (codepoint >>  0)));
				break;
			}
		}
	}

	return size;
}

// IMPORTANT(fusion): This function WON'T handle null-termination. It'll rather
// convert any characters, INCLUDING the null-terminator, contained in the src
// string. Invalid or NON-LATIN1 codepoints are translated into '?'.
int utf8_to_latin1(char* dest, int dest_capacity, const char* src, int src_length){
	int read_pos = 0;
	int write_pos = 0;
	while(read_pos < src_length){
		int codepoint = -1;
		int size = utf8_decode_one((uint8*)(src + read_pos), (src_length - read_pos), &codepoint);
		if(size > 0){
			read_pos += size;
		}else{
			read_pos += utf8_find_next_leading_byte((src + read_pos), (src_length - read_pos));
		}

		if(write_pos < dest_capacity){
			if(codepoint >= 0 && codepoint <= 0xFF){
				dest[write_pos] = (char)codepoint;
			}else{
				dest[write_pos] = '?';
			}
		}
		write_pos += 1;
	}

	return write_pos;
}

// IMPORTANT(fusion): This function WON'T handle null-termination. It'll rather
// convert any characters, INCLUDING the null-terminator, contained in the src
// string. Note that LATIN1 characters translates directly into UNICODE codepoints.
int latin1_to_utf8(char* dest, int dest_capacity, const char* src, int src_length){
	int write_pos = 0;
	for(int read_pos = 0; read_pos < src_length; read_pos += 1){
		write_pos += utf8_encode_one((uint8*)(dest + write_pos),
				(dest_capacity - write_pos), (uint8)src[read_pos]);
	}
	return write_pos;
}
