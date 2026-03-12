#ifndef LOGINSERVER_STATUS_XML_BUILDER_H
#define LOGINSERVER_STATUS_XML_BUILDER_H

#include "common/types.h"
#include "common/assert.h"
#include <stdarg.h>

struct XmlBuffer {
    char* data;
    int size;
    int position;
};

void xml_null_terminate(XmlBuffer* buffer);
void xml_append_char(XmlBuffer* buffer, char ch);
void xml_append_number(XmlBuffer* buffer, int64 num);
void xml_append_string(XmlBuffer* buffer, const char* str);
void xml_append_string_escaped(XmlBuffer* buffer, const char* str);
void xml_append_format_v(XmlBuffer* buffer, const char* format, va_list args);
void xml_append_format(XmlBuffer* buffer, const char* format, ...) ATTR_PRINTF(2, 3);

#endif // LOGINSERVER_STATUS_XML_BUILDER_H
