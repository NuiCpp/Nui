project(webview-git NONE)

if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(
        webkit2 REQUIRED webkit2gtk-4.0
        IMPORTED_TARGET
    )
endif()

include(FetchContent)
FetchContent_Declare(
    webview_raw
    GIT_REPOSITORY https://github.com/webview/webview.git
    GIT_TAG        6117dbf03f4a1a465b4135a42a84d33aeec71d7f    
)

FetchContent_MakeAvailable(webview_raw)

if (WIN32)
add_custom_target(
    webview2_win
    ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/_deps/webview2" 
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
    COMMAND curl -sSL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2" -o temp.zip
    COMMAND unzip -v -n "${CMAKE_BINARY_DIR}/temp.zip" -d "${CMAKE_BINARY_DIR}/_deps/webview2"
    COMMAND rm "${CMAKE_BINARY_DIR}/temp.zip"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview2/build/native/x64/WebView2Loader.dll" "${CMAKE_BINARY_DIR}/bin"
    COMMAND_EXPAND_LISTS
)
endif()

add_custom_target(
    webview_prep 
    ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include" 
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview_raw-src/webview.h" "${CMAKE_BINARY_DIR}/include/webview.h"
    COMMAND_EXPAND_LISTS
)

add_library(webview INTERFACE)
target_include_directories(webview INTERFACE "${CMAKE_BINARY_DIR}/include")

if (WIN32)
    target_include_directories(webview INTERFACE "${CMAKE_BINARY_DIR}/_deps/webview2/build/native/include")
    target_link_directories(webview INTERFACE "${CMAKE_BINARY_DIR}/_deps/webview2/build/native/x64")
endif()

add_dependencies(webview webview_prep)
set_target_properties(webview PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:webview,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(
    webview
    INTERFACE
        $<$<PLATFORM_ID:Linux>:
            PkgConfig::webkit2
        >
)

if (WIN32)
    add_dependencies(webview webview2_win)
endif()