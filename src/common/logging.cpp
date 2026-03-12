#include "logging.h"
#include "string_utils.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

static int64 start_time_ms = 0;

void init_monotonic_clock() {
    start_time_ms = get_monotonic_clock_ms();
}

void log_info(const char* prefix, const char* format, ...) {
    char entry[4096];
    va_list ap;
    va_start(ap, format);
    vsnprintf(entry, sizeof(entry), format, ap);
    va_end(ap);

    // Trim trailing whitespace.
    int length = static_cast<int>(strlen(entry));
    while (length > 0 && isspace(entry[length - 1])) {
        entry[length - 1] = 0;
        length -= 1;
    }

    if (length > 0) {
        char time_string[128];
        string_buf_format_time(time_string, "%Y-%m-%d %H:%M:%S", static_cast<int>(time(nullptr)));
        fprintf(stdout, "%s [%s] %s\n", time_string, prefix, entry);
        fflush(stdout);
    }
}

void log_verbose(const char* prefix, const char* function,
        const char* file, int line, const char* format, ...) {
    char entry[4096];
    va_list ap;
    va_start(ap, format);
    vsnprintf(entry, sizeof(entry), format, ap);
    va_end(ap);

    // Trim trailing whitespace.
    int length = static_cast<int>(strlen(entry));
    while (length > 0 && isspace(entry[length - 1])) {
        entry[length - 1] = 0;
        length -= 1;
    }

    if (length > 0) {
        (void)file;
        (void)line;
        char time_string[128];
        string_buf_format_time(time_string, "%Y-%m-%d %H:%M:%S", static_cast<int>(time(nullptr)));
        fprintf(stdout, "%s [%s] %s: %s\n", time_string, prefix, function, entry);
        fflush(stdout);
    }
}

struct tm get_local_time(time_t t) {
    struct tm result;
    localtime_r(&t, &result);
    return result;
}

struct tm get_gm_time(time_t t) {
    struct tm result;
    gmtime_r(&t, &result);
    return result;
}

int64 get_monotonic_clock_ms() {
    struct timespec time_spec;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &time_spec);
    return (static_cast<int64>(time_spec.tv_sec) * 1000)
        + (static_cast<int64>(time_spec.tv_nsec) / 1000000);
}

int get_monotonic_uptime() {
    return static_cast<int>((get_monotonic_clock_ms() - start_time_ms) / 1000);
}
