cmake_minimum_required(VERSION 3.16)

project(patch-dotenv VERSION 0.1.0)

add_executable(patch-dotenv main.cpp)
target_compile_features(patch-dotenv PRIVATE cxx_std_20)

set_target_properties(patch-dotenv
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)