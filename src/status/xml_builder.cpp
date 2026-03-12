#include "status/xml_builder.h"
#include "common/assert.h"
#include "common/logging.h"

void xml_null_terminate(XmlBuffer* buffer) {
    ASSERT(buffer->size > 0);
    if(buffer->position < buffer->size) {
        buffer->data[buffer->position] = 0;
    } else {
        buffer->data[buffer->size - 1] = 0;
    }
}

void xml_append_char(XmlBuffer* buffer, char ch) {
    if(buffer->position < buffer->size) {
        buffer->data[buffer->position] = ch;
    }
    buffer->position += 1;
}

void xml_append_number(XmlBuffer* buffer, int64 num) {
    if(num == 0) {
        xml_append_char(buffer, '0');
        return;
    }

    if(num < 0) {
        xml_append_char(buffer, '-');
        num = -num;
    }

    char string[64] = {};
    int string_len = 0;
    while(num > 0) {
        ASSERT(string_len < (int)sizeof(string));
        string[string_len] = (num % 10) + '0';
        num = (num / 10);
        string_len += 1;
    }

    for(int i = 0; i < string_len; i += 1) {
        xml_append_char(buffer, string[(string_len - 1) - i]);
    }
}

void xml_append_string(XmlBuffer* buffer, const char* str) {
    const char* p = str;
    while(p[0]) {
        xml_append_char(buffer, p[0]);
        p += 1;
    }
}

void xml_append_string_escaped(XmlBuffer* buffer, const char* str) {
    const char* p = str;
    while(p[0]) {
        switch(p[0]) {
            case '\t': xml_append_string(buffer, "&#9;"); break;
            case '\n': xml_append_string(buffer, "&#10;"); break;
            case '"':  xml_append_string(buffer, "&quot;"); break;
            case '&':  xml_append_string(buffer, "&amp;"); break;
            case '\'': xml_append_string(buffer, "&apos;"); break;
            case '<':  xml_append_string(buffer, "&lt;"); break;
            case '>':  xml_append_string(buffer, "&gt;"); break;
            default:   xml_append_char(buffer, p[0]); break;
        }
        p += 1;
    }
}

void xml_append_format_v(XmlBuffer* buffer, const char* format, va_list args) {
    const char* p = format;
    while(p[0]) {
        // IMPORTANT(fusion): This function only implements a small subset of
        // the printf syntax.
        if(p[0] == '%') {
            p += 1;

            // IMPORTANT(fusion): Small integral parameters such as char and short
            // are promoted to int, similar to how they're promoted in arithmetic
            // operations. This means there are only really 4 and 8 bytes integers.
            int length = 4;
            if(p[0] == 'h' && p[1] == 'h') {
                p += 2;
            } else if(p[0] == 'l' && p[1] == 'l') {
                length = 8;
                p += 2;
            } else if(p[0] == 'h') {
                p += 1;
            } else if(p[0] == 'l') {
                length = 8;
                p += 1;
            }

            switch(p[0]) {
                case '%': {
                    xml_append_char(buffer, '%');
                    break;
                }

                case 'c': {
                    int ch = va_arg(args, int);
                    xml_append_char(buffer, (char)ch);
                    break;
                }

                case 'd': {
                    int64 num = (length == 4 ? va_arg(args, int) : va_arg(args, int64));
                    xml_append_number(buffer, num);
                    break;
                }

                case 's': {
                    const char* str = va_arg(args, const char*);
                    xml_append_string_escaped(buffer, str);
                    break;
                }

                default: {
                    LOG_ERR("Invalid XML format specifier \"%c\"", p[0]);
                    break;
                }
            }

            if(p[0]) {
                p += 1;
            }

        } else {
            xml_append_char(buffer, p[0]);
            p += 1;
        }
    }
}

void xml_append_format(XmlBuffer* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    xml_append_format_v(buffer, format, args);
    va_end(args);
}
