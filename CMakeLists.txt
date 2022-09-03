cmake_minimum_required(VERSION 3.21)

project(nui VERSION 0.1.0)

include(./cmake/common.cmake)
include(./cmake/options.cmake)
include(./cmake/dependencies/boostpp.cmake)
if (EMSCRIPTEN)
    message("CMake is called from emscripten")
    include(./cmake/dependencies/libcpppre.cmake)
    include(./cmake/frontend/common.cmake)
    include(./cmake/frontend/options.cmake)
    include(./cmake/frontend/emscripten.cmake)
else()
    add_subdirectory(./tools/bin2hpp)
    include(./cmake/backend/common.cmake)
    include(./cmake/backend/options.cmake)
    include(./cmake/backend/emscripten.cmake)
    include(./cmake/dependencies/webview.cmake)
    include(./cmake/dependencies/fmt.cmake)
endif()

# Submodules

# Project Native
add_subdirectory(nui/src/nui)

if (${NUI_BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()