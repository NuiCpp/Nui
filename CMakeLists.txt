cmake_minimum_required(VERSION 3.21)

project(nui VERSION 0.1.0)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/boostpp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/mplex.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/interval_tree.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/fmt.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/describe.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/mp11.cmake)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/patch_acorn)
if (EMSCRIPTEN)
    message("CMake is called from emscripten")
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/libcpppre.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/common.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/options.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/emscripten.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/nlohmann_json.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/portable_file_dialog.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/webview.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/roar.cmake)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/bin2hpp)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/parcel_adapter)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/common.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/options.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/emscripten.cmake)
endif()

# Submodules

# Project Native
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nui/src/nui)

if (NUI_ENABLE_TESTS)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/gtest.cmake)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nui/test/nui)
endif()

if (${NUI_BUILD_EXAMPLES})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples)
endif()