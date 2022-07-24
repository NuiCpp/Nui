cmake_minimum_required(VERSION 3.21)

project(nui VERSION 0.1.0)

include(./cmake/common.cmake)
include(./cmake/options.cmake)
if (EMSCRIPTEN)
    include(./cmake/frontend/common.cmake)
    include(./cmake/frontend/options.cmake)
    include(./cmake/frontend/emscripten.cmake)
else()
    include(./cmake/backend/common.cmake)
    include(./cmake/backend/options.cmake)
    include(./cmake/backend/emscripten.cmake)
    include(./cmake/webview.cmake)
endif()

# Submodules

# Project Native
add_subdirectory(nui/src/nui)

if (${NUI_BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()