#ifndef LOGINSERVER_COMMON_LOGGING_H
#define LOGINSERVER_COMMON_LOGGING_H

#include "assert.h"
#include "types.h"

#include <ctime>

void log_info(const char* prefix, const char* format, ...) ATTR_PRINTF(2, 3);
void log_verbose(const char* prefix, const char* function,
    const char* file, int line, const char* format, ...) ATTR_PRINTF(5, 6);

#define LOG(...)      log_info("INFO", __VA_ARGS__)
#define LOG_WARN(...) log_verbose("WARN", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERR(...)  log_verbose("ERR", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define PANIC(...)                                                              \
    do {                                                                        \
        log_verbose("PANIC", __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);    \
        TRAP();                                                                 \
    } while(0)

struct tm get_local_time(time_t t);
struct tm get_gm_time(time_t t);
int64 get_monotonic_clock_ms();
int get_monotonic_uptime();

// Must be called at startup to initialize the monotonic baseline.
void init_monotonic_clock();

#endif // LOGINSERVER_COMMON_LOGGING_H
