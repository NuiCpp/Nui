include(GoogleTest)

find_package(Boost CONFIG REQUIRED)

add_executable(inline_parser-tests
    tests.cpp
    temp_dir.cpp
)
target_link_libraries(inline_parser-tests
    PRIVATE
        inline-parser-lib
        gtest
)
gtest_discover_tests(inline_parser-tests)

set_target_properties(inline_parser-tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)