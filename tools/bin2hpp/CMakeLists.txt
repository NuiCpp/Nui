cmake_minimum_required(VERSION 3.16)

project(bin2hpp VERSION 0.1.0)

add_executable(bin2hpp main.cpp raw_encoder.cpp base64_encoder.cpp compressed_encoder.cpp)
target_compile_features(bin2hpp PRIVATE cxx_std_20)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/dependencies/boost.cmake")

set_target_properties(bin2hpp
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)

target_link_libraries(bin2hpp PRIVATE roar Boost::iostreams)