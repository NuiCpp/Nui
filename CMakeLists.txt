cmake_minimum_required(VERSION 3.28)

project(nui VERSION 1.1.0)

set(NUI_SOURCE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} CACHE INTERNAL "NUI source directory")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/options.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/static_analyzers.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/inline_extractor.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/boostpp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/interval_tree.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/fmt.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/describe.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/mp11.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/nui_traits.cmake)
if (EMSCRIPTEN)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/frontend/emscripten.cmake)
else()
    add_library(nui-base64 INTERFACE)
    target_include_directories(nui-base64 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/nui/include")

    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/bin2hpp)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/parcel_adapter)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/patch_dotenv)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/inline_parser)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/inline_injector)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/webview_uuid)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/binaryen.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/emscripten.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/nlohmann_json.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/portable_file_dialog.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/webview.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/roar.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/common.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/backend/emscripten.cmake)

    if (${NUI_BUILD_XML_TOOL})
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/xml_to_nui)
    endif()
endif()

# Submodules

# Project Native
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nui/src/nui)

if (${NUI_ENABLE_TESTS})
    enable_testing()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/dependencies/gtest.cmake)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nui/test/nui)
endif()

if (${NUI_BUILD_EXAMPLES})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/examples)
endif()