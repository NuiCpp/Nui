cmake_minimum_required(VERSION 3.21)

project(parcel-adapter VERSION 0.1.0)

add_executable(parcel-adapter main.cpp)
target_link_libraries(parcel-adapter PRIVATE nlohmann_json)