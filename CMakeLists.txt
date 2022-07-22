cmake_minimum_required(VERSION 3.21)

project(nui VERSION 0.1.0)

include(./cmake/options.cmake)
include(./cmake/common.cmake)
include(./cmake/emscripten.cmake)
include(./cmake/webview.cmake)

# Submodules

# Project Native
add_subdirectory(nui/src/nui)

if (${NUI_BUILD_EXAMPLES})
    add_subdirectory(nui/examples)
endif()