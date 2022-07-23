cmake_minimum_required(VERSION 3.21)

project(nui-basic VERSION 0.1.0)

set(COMMON_OPTIONS -fexceptions -Wall -pedantic-errors -pedantic)
set(DEBUG_OPTIONS  -g ${COMMON_OPTIONS})
set(RELEASE_OPTIONS -O3 ${COMMON_OPTIONS})

add_executable(nui-basic main.cpp)

target_compile_options(nui-basic PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(nui-basic PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

#nui_set_project_warnings(nui-basic)    

if (EMSCRIPTEN)
    target_link_libraries(nui-basic PRIVATE nui-frontend)
    set_target_properties(nui-basic PROPERTIES LINK_FLAGS "-s DEMANGLE_SUPPORT=1 --no-entry --bind -s NO_EXIT_RUNTIME=1 --pre-js=\"../cmake/emsdk_prejs.js\"")
else()
    target_link_libraries(nui-basic PRIVATE nui-backend)
    nui_add_emscripten_target(nui-basic)
    # Use this to add more source files based on frontend/backend build
    # target_sources(nui-basic PRIVATE ${extra_file})
    
endif()
