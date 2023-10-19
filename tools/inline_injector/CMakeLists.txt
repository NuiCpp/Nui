cmake_minimum_required(VERSION 3.16)

project(inline-injector VERSION 0.1.0)

add_executable(inline-injector main.cpp)
target_compile_features(inline-injector PRIVATE cxx_std_20)

set_target_properties(inline-injector
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)