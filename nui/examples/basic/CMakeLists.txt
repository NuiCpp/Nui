cmake_minimum_required(VERSION 3.21)

project(nui-basic VERSION 0.1.0)

add_executable(nui-basic-b main.cpp)
target_link_libraries(nui-basic-b PRIVATE nui-backend)

apply_project_properties(nui-basic-b)

set(COMMON_OPTIONS -fexceptions -g -Wall -pedantic-errors -pedantic)
set(DEBUG_OPTIONS ${COMMON_OPTIONS})
set(RELEASE_OPTIONS -O3 ${COMMON_OPTIONS})
target_compile_options(nui-basic-b PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(nui-basic-b PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

# TODO: add cool cmake functions for this backend/frontend thing
if (EMSCRIPTEN)
    add_executable(nui-basic-f main.cpp)
    target_link_libraries(nui-basic-f PRIVATE nui-frontend)
    set_target_properties(nui-basic-f PROPERTIES LINK_FLAGS "-s DEMANGLE_SUPPORT=1 --no-entry --bind -s NO_EXIT_RUNTIME=1 --pre-js=\"../cmake/emsdk_prejs.js\"")
endif()