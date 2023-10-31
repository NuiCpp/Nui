cmake_minimum_required(VERSION 3.16)

project(webview-uuid VERSION 0.1.0)

add_executable(webview-uuid main.cpp)
target_compile_features(webview-uuid PRIVATE cxx_std_20)

set_target_properties(webview-uuid
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)