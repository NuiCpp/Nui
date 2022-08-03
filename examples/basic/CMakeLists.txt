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
    # Link frontend of nui inside of emscripten
    target_link_libraries(nui-basic PRIVATE nui-frontend)

    # Sets additional properties etc on the target.
    nui_prepare_emscripten_target(nui-basic)    
else()
    # Link backend of nui outside of emscripten
    target_link_libraries(nui-basic PRIVATE nui-backend)

    nui_set_target_output_directories(nui-basic)
    
    # Creates a target that is compiled through emscripten. This target becomes the frontend part.
    nui_add_emscripten_target(nui-basic ${CMAKE_SOURCE_DIR} -DNUI_BUILD_EXAMPLES=on)

    # Use this to add more source files based on frontend/backend build
    # target_sources(nui-basic PRIVATE ${extra_file})
endif()
