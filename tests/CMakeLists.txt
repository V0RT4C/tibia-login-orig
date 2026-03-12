# Tests will be added incrementally as modules are extracted.
# Each test file links against the modules it tests.

add_executable(test_byte_order test_byte_order.cpp)
target_include_directories(test_byte_order PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_byte_order PRIVATE cxx_std_17)
add_test(NAME test_byte_order COMMAND test_byte_order)

add_executable(test_utf8 test_utf8.cpp ../src/common/utf8.cpp)
target_include_directories(test_utf8 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_utf8 PRIVATE cxx_std_17)
add_test(NAME test_utf8 COMMAND test_utf8)

add_executable(test_buffer_reader test_buffer_reader.cpp
    ../src/common/buffer_reader.cpp ../src/common/utf8.cpp)
target_include_directories(test_buffer_reader PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_buffer_reader PRIVATE cxx_std_17)
add_test(NAME test_buffer_reader COMMAND test_buffer_reader)

add_executable(test_buffer_writer test_buffer_writer.cpp
    ../src/common/buffer_writer.cpp ../src/common/buffer_reader.cpp ../src/common/utf8.cpp)
target_include_directories(test_buffer_writer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_buffer_writer PRIVATE cxx_std_17)
add_test(NAME test_buffer_writer COMMAND test_buffer_writer)

add_executable(test_string_utils test_string_utils.cpp ../src/common/string_utils.cpp)
target_include_directories(test_string_utils PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_string_utils PRIVATE cxx_std_17)
add_test(NAME test_string_utils COMMAND test_string_utils)

add_executable(test_config test_config.cpp ../src/config/config.cpp
    ../src/common/string_utils.cpp ../src/common/logging.cpp ../src/common/utf8.cpp)
target_include_directories(test_config PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_config PRIVATE cxx_std_17)
add_test(NAME test_config COMMAND test_config)

add_executable(test_xtea test_xtea.cpp ../src/crypto/xtea.cpp)
target_include_directories(test_xtea PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_xtea PRIVATE cxx_std_17)
add_test(NAME test_xtea COMMAND test_xtea)

add_executable(test_xml_builder test_xml_builder.cpp
    ../src/status/xml_builder.cpp ../src/common/logging.cpp ../src/common/string_utils.cpp)
target_include_directories(test_xml_builder PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_xml_builder PRIVATE cxx_std_17)
add_test(NAME test_xml_builder COMMAND test_xml_builder)

# Suppress OpenSSL deprecated API warnings for rsa.cpp in test targets
set_source_files_properties(../src/crypto/rsa.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")

add_executable(test_rsa test_rsa.cpp ../src/crypto/rsa.cpp
    ../src/common/logging.cpp ../src/common/string_utils.cpp)
target_include_directories(test_rsa PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_rsa PRIVATE cxx_std_17)
target_compile_options(test_rsa PRIVATE -Wno-deprecated-declarations)
target_link_libraries(test_rsa PRIVATE OpenSSL::Crypto)
add_test(NAME test_rsa COMMAND test_rsa)

add_executable(test_logging test_logging.cpp
    ../src/common/logging.cpp ../src/common/string_utils.cpp)
target_include_directories(test_logging PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_logging PRIVATE cxx_std_17)
add_test(NAME test_logging COMMAND test_logging)

add_executable(test_login_handler test_login_handler.cpp
    ../src/login/login_handler.cpp
    ../src/common/buffer_reader.cpp ../src/common/buffer_writer.cpp
    ../src/common/utf8.cpp ../src/common/string_utils.cpp
    ../src/common/logging.cpp ../src/crypto/xtea.cpp ../src/crypto/rsa.cpp
    ../src/query/query_client.cpp)
target_include_directories(test_login_handler PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_features(test_login_handler PRIVATE cxx_std_17)
target_compile_options(test_login_handler PRIVATE -Wno-deprecated-declarations)
target_link_libraries(test_login_handler PRIVATE OpenSSL::Crypto)
add_test(NAME test_login_handler COMMAND test_login_handler)
