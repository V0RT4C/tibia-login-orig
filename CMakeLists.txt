cmake_minimum_required(VERSION 3.16)
project(loginserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASSERTIONS "Enable runtime assertions" OFF)
option(CLIENT_ENCODING_UTF8 "Client uses UTF-8 encoding" OFF)
option(TIBIA772 "Target Tibia 7.72 protocol" OFF)

# Main executable
set(SOURCES
    src/main.cpp
    src/connections.cpp
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
)

add_executable(login ${SOURCES})

target_include_directories(login PRIVATE src)

target_compile_options(login PRIVATE
    -m64
    -fno-strict-aliasing
    -Wall
    -Wextra
    -pedantic
)

# Suppress OpenSSL deprecated API warnings for the RSA compilation unit only
set_source_files_properties(src/crypto/rsa.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")

target_compile_definitions(login PRIVATE
    $<$<BOOL:${ENABLE_ASSERTIONS}>:ENABLE_ASSERTIONS=1>
    $<$<BOOL:${CLIENT_ENCODING_UTF8}>:CLIENT_ENCODING_UTF8=1>
    $<$<BOOL:${TIBIA772}>:TIBIA772=1>
)

find_package(OpenSSL REQUIRED)
target_link_libraries(login PRIVATE OpenSSL::Crypto pthread)

# Tests
enable_testing()
add_subdirectory(tests)
