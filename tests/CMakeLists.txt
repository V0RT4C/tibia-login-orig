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
