cmake_minimum_required(VERSION 3.16)

project(inline-parser VERSION 0.1.0)

include("${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/dependencies/nlohmann_json.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/dependencies/boost.cmake")

add_library(inline-parser-lib
    STATIC
        inline_extractor.cpp
        section_cache.cpp
)
target_link_libraries(inline-parser-lib PUBLIC nlohmann_json::nlohmann_json nui-base64)
target_compile_features(inline-parser-lib PUBLIC cxx_std_20)
target_include_directories(inline-parser-lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include ${Boost_INCLUDE_DIRS})

add_executable(inline-parser main.cpp)
target_link_libraries(inline-parser PRIVATE inline-parser-lib)

set_target_properties(inline-parser
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)