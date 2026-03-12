#ifndef LOGINSERVER_COMMON_ASSERT_H
#define LOGINSERVER_COMMON_ASSERT_H

#include <cstdlib>

#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(__clang__)
#define COMPILER_CLANG 1
#endif

#if defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
#define OS_LINUX 1
#else
#error "Operating system not supported."
#endif

#if COMPILER_GCC || COMPILER_CLANG
#define ATTR_FALLTHROUGH __attribute__((fallthrough))
#define ATTR_PRINTF(x, y) __attribute__((format(printf, x, y)))
#else
#define ATTR_FALLTHROUGH
#define ATTR_PRINTF(x, y)
#endif

#if COMPILER_MSVC
#define TRAP() __debugbreak()
#elif COMPILER_GCC || COMPILER_CLANG
#define TRAP() __builtin_trap()
#else
#define TRAP() abort()
#endif

#define ASSERT_ALWAYS(expr) if(!(expr)) { TRAP(); }

#if ENABLE_ASSERTIONS
#define ASSERT(expr) ASSERT_ALWAYS(expr)
#else
#define ASSERT(expr) ((void)(expr))
#endif

#endif // LOGINSERVER_COMMON_ASSERT_H
