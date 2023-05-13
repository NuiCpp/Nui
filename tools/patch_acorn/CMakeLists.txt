cmake_minimum_required(VERSION 3.21)

project(patch-acorn VERSION 0.1.0)

add_executable(patch-acorn main.cpp)
target_compile_features(patch-acorn PRIVATE cxx_std_20)