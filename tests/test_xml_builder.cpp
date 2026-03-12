#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "status/xml_builder.h"

#include <cstring>

TEST_CASE("xml_null_terminate within bounds") {
    char buf[16] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 5};
    xml_null_terminate(&xml);
    CHECK(buf[5] == '\0');
}

TEST_CASE("xml_null_terminate at capacity") {
    char buf[4] = {'A', 'B', 'C', 'D'};
    XmlBuffer xml = {buf, 4, 10}; // position past size
    xml_null_terminate(&xml);
    CHECK(buf[3] == '\0'); // last byte
}

TEST_CASE("xml_append_char") {
    char buf[16] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_char(&xml, 'H');
    xml_append_char(&xml, 'i');
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "Hi") == 0);
    CHECK(xml.position == 2);
}

TEST_CASE("xml_append_number positive") {
    char buf[32] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_number(&xml, 12345);
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "12345") == 0);
}

TEST_CASE("xml_append_number zero") {
    char buf[32] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_number(&xml, 0);
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "0") == 0);
}

TEST_CASE("xml_append_number negative") {
    char buf[32] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_number(&xml, -42);
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "-42") == 0);
}

TEST_CASE("xml_append_string") {
    char buf[32] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_string(&xml, "hello");
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "hello") == 0);
}

TEST_CASE("xml_append_string_escaped") {
    char buf[64] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_string_escaped(&xml, "<test>&\"'");
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "&lt;test&gt;&amp;&quot;&apos;") == 0);
}

TEST_CASE("xml_append_format with string and int") {
    char buf[128] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_format(&xml, "<player name=\"%s\" level=\"%d\"/>", "Test&Name", 100);
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "<player name=\"Test&amp;Name\" level=\"100\"/>") == 0);
}

TEST_CASE("xml_append_format percent escape") {
    char buf[32] = {};
    XmlBuffer xml = {buf, (int)sizeof(buf), 0};
    xml_append_format(&xml, "100%%");
    xml_null_terminate(&xml);
    CHECK(std::strcmp(buf, "100%") == 0);
}
