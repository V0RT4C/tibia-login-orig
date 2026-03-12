#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/common/logging.h"

#include <ctime>

TEST_CASE("get_monotonic_clock_ms returns positive value") {
    int64 ms = get_monotonic_clock_ms();
    CHECK(ms > 0);
}

TEST_CASE("get_monotonic_clock_ms is non-decreasing") {
    int64 a = get_monotonic_clock_ms();
    int64 b = get_monotonic_clock_ms();
    CHECK(b >= a);
}

TEST_CASE("init_monotonic_clock and get_monotonic_uptime") {
    init_monotonic_clock();
    int uptime = get_monotonic_uptime();
    CHECK(uptime >= 0);
    CHECK(uptime < 5); // should be near zero right after init
}

TEST_CASE("get_local_time returns valid struct") {
    time_t now = time(nullptr);
    struct tm result = get_local_time(now);
    CHECK(result.tm_year >= 125); // 2025+
    CHECK(result.tm_mon >= 0);
    CHECK(result.tm_mon <= 11);
    CHECK(result.tm_mday >= 1);
    CHECK(result.tm_mday <= 31);
    CHECK(result.tm_hour >= 0);
    CHECK(result.tm_hour <= 23);
}

TEST_CASE("get_gm_time returns valid struct") {
    time_t now = time(nullptr);
    struct tm result = get_gm_time(now);
    CHECK(result.tm_year >= 125);
    CHECK(result.tm_mon >= 0);
    CHECK(result.tm_mon <= 11);
    CHECK(result.tm_mday >= 1);
    CHECK(result.tm_mday <= 31);
}

TEST_CASE("get_local_time for known epoch value") {
    // 2000-01-01 00:00:00 UTC = 946684800
    time_t epoch_2000 = 946684800;
    struct tm result = get_gm_time(epoch_2000);
    CHECK(result.tm_year == 100); // years since 1900
    CHECK(result.tm_mon == 0);    // January
    CHECK(result.tm_mday == 1);
    CHECK(result.tm_hour == 0);
    CHECK(result.tm_min == 0);
    CHECK(result.tm_sec == 0);
}
