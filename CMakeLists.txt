cmake_minimum_required(VERSION 3.21)

project(nui VERSION 0.1.0)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/boostpp.cmake)
if (EMSCRIPTEN)
    message("CMake is called from emscripten")
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/libcpppre.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/common.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/options.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/emscripten.cmake)
else()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/bin2hpp)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/common.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/options.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/emscripten.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/webview.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/fmt.cmake)
endif()

# Submodules

# Project Native
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nui/src/nui)

if (${NUI_BUILD_EXAMPLES})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples)
endif()