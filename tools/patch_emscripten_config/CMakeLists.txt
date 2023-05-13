cmake_minimum_required(VERSION 3.21)

project(patch-emscripten-config VERSION 0.1.0)

add_executable(patch-emscripten-config main.cpp)
target_compile_features(patch-emscripten-config PRIVATE cxx_std_20)