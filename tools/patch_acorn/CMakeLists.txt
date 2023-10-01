cmake_minimum_required(VERSION 3.16)

project(patch-acorn VERSION 0.1.0)

add_executable(patch-acorn main.cpp)
target_compile_features(patch-acorn PRIVATE cxx_std_20)

set_target_properties(patch-acorn
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools"
)