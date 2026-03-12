#ifndef LOGINSERVER_COMMON_TYPES_H
#define LOGINSERVER_COMMON_TYPES_H

#include <cstddef>
#include <cstdint>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int64 = int64_t;

constexpr int kb(int x) { return x * 1024; }

#ifndef CLIENT_ENCODING_UTF8
#define CLIENT_ENCODING_UTF8 0
#endif

#ifndef TIBIA772
#define TIBIA772 0
#endif

#endif // LOGINSERVER_COMMON_TYPES_H
