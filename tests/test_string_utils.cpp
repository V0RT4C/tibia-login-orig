#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "common/string_utils.h"

#include <cstring>

TEST_CASE("string_empty") {
    CHECK(string_empty("") == true);
    CHECK(string_empty("a") == false);
}

TEST_CASE("string_equals") {
    CHECK(string_equals("abc", "abc") == true);
    CHECK(string_equals("abc", "def") == false);
    CHECK(string_equals("abc", "abcd") == false);
    CHECK(string_equals("", "") == true);
}

TEST_CASE("string_equals_ignore_case") {
    CHECK(string_equals_ignore_case("ABC", "abc") == true);
    CHECK(string_equals_ignore_case("Hello", "HELLO") == true);
    CHECK(string_equals_ignore_case("abc", "def") == false);
}

TEST_CASE("string_copy") {
    char dest[8];
    CHECK(string_copy(dest, sizeof(dest), "Hello") == true);
    CHECK(std::strcmp(dest, "Hello") == 0);

    // Too long
    CHECK(string_copy(dest, sizeof(dest), "TooLongString") == false);
    CHECK(dest[0] == 0); // cleared on failure
}

TEST_CASE("string_copy_n") {
    char dest[8];
    CHECK(string_copy_n(dest, sizeof(dest), "Hello World", 5) == true);
    CHECK(std::strcmp(dest, "Hello") == 0);
}

TEST_CASE("string_format") {
    char dest[32];
    CHECK(string_format(dest, sizeof(dest), "Hello %s %d", "world", 42) == true);
    CHECK(std::strcmp(dest, "Hello world 42") == 0);
}

TEST_CASE("string_clear") {
    char dest[4] = {'a', 'b', 'c', 'd'};
    string_clear(dest, sizeof(dest));
    CHECK(dest[0] == 0);
    CHECK(dest[3] == 0);
}

TEST_CASE("string_hash - deterministic") {
    uint32 h1 = string_hash("test");
    uint32 h2 = string_hash("test");
    CHECK(h1 == h2);
    CHECK(h1 != 0);
}

TEST_CASE("string_hash - different strings differ") {
    CHECK(string_hash("abc") != string_hash("def"));
}

TEST_CASE("string_escape") {
    char dest[32];
    CHECK(string_escape(dest, sizeof(dest), "Hello\nWorld") == true);
    CHECK(std::strcmp(dest, "Hello\\nWorld") == 0);
}

TEST_CASE("string_escape - tab and backslash") {
    char dest[32];
    string_escape(dest, sizeof(dest), "A\tB\\C");
    CHECK(std::strcmp(dest, "A\\tB\\\\C") == 0);
}
