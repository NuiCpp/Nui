cmake_minimum_required(VERSION 3.16)

project(parcel-adapter VERSION 0.1.0)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/dependencies/nlohmann_json.cmake")

add_executable(parcel-adapter main.cpp)
target_link_libraries(parcel-adapter PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(parcel-adapter PRIVATE cxx_std_20)

set_target_properties(parcel-adapter
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)