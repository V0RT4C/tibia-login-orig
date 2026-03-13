cmake_minimum_required(VERSION 3.16)
project(loginserver LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# uSockets — lightweight C networking library (dependency of uWebSockets)
# Explicit file list — do NOT glob. Exclude crypto/openssl.c (we use LIBUS_NO_SSL)
# and quic.c (has extra dependencies we don't need).
set(USOCKETS_SOURCES
    third_party/uSockets/src/bsd.c
    third_party/uSockets/src/context.c
    third_party/uSockets/src/loop.c
    third_party/uSockets/src/socket.c
    third_party/uSockets/src/udp.c
    third_party/uSockets/src/eventing/epoll_kqueue.c
    third_party/uSockets/src/crypto/sni_tree.cpp
)
add_library(uSockets STATIC ${USOCKETS_SOURCES})
target_include_directories(uSockets PUBLIC third_party/uSockets/src)
target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL=1)
target_compile_options(uSockets PRIVATE -Wno-unused-parameter -Wno-pedantic)
target_link_libraries(uSockets PRIVATE z)

option(ENABLE_ASSERTIONS "Enable runtime assertions" OFF)
option(CLIENT_ENCODING_UTF8 "Client uses UTF-8 encoding" OFF)
option(TIBIA772 "Target Tibia 7.72 protocol" OFF)

# Main executable
set(SOURCES
    src/main.cpp
    src/network/server.cpp
    src/login/login_handler.cpp
    src/query/login_query.cpp
    src/query/world_query.cpp
    src/query/query_client.cpp
    src/status/xml_builder.cpp
    src/status/status_handler.cpp
    src/crypto/xtea.cpp
    src/crypto/rsa.cpp
    src/config/config.cpp
    src/common/logging.cpp
    src/common/string_utils.cpp
    src/common/utf8.cpp
    src/common/buffer_reader.cpp
    src/common/buffer_writer.cpp
    src/network/websocket_acceptor.cpp
)

add_executable(login ${SOURCES})

target_include_directories(login PRIVATE src)
target_include_directories(login PRIVATE third_party/uWebSockets/src)

target_compile_options(login PRIVATE
    -m64
    -fno-strict-aliasing
    -Wall
    -Wextra
    -pedantic
)

# Suppress OpenSSL deprecated API warnings for the RSA compilation unit only
set_source_files_properties(src/crypto/rsa.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")

# Suppress C++20 designated initializer warnings for the WebSocket acceptor (uses uWS API style)
set_source_files_properties(src/network/websocket_acceptor.cpp PROPERTIES COMPILE_FLAGS "-Wno-pedantic")

target_compile_definitions(login PRIVATE
    $<$<BOOL:${ENABLE_ASSERTIONS}>:ENABLE_ASSERTIONS=1>
    $<$<BOOL:${CLIENT_ENCODING_UTF8}>:CLIENT_ENCODING_UTF8=1>
    $<$<BOOL:${TIBIA772}>:TIBIA772=1>
)

find_package(OpenSSL REQUIRED)
target_link_libraries(login PRIVATE OpenSSL::Crypto pthread uSockets)

# Tests
enable_testing()
add_subdirectory(tests)
