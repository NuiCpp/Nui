project(webview-git NONE)

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(
        webkit2 REQUIRED webkit2gtk-4.0
        IMPORTED_TARGET
    )
endif()

include(FetchContent)
FetchContent_Declare(
    webview_raw
    GIT_REPOSITORY https://github.com/5cript/webview.git
    GIT_TAG        5002fb32b3c6981e5aae9bcee6f07a7dc3ce3dac
)

FetchContent_MakeAvailable(webview_raw)

if (WIN32)
    FetchContent_Declare(
          webview_binary
          URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2"
          DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_GetProperties(webview_binary)
    if (webview_binary_POPULATED)
    else()
        FetchContent_Populate(webview_binary)
    endif()

    FetchContent_Declare(
        binaryen_release
        URL "https://github.com/WebAssembly/binaryen/releases/download/version_112/binaryen-version_112-x86_64-windows.tar.gz"
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_GetProperties(binaryen_release)
    if (binaryen_release_POPULATED)
	else()
		FetchContent_Populate(binaryen_release)
    endif()

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WEBVIEW_DLL_SUBDIRECTORY "x64")
    else()
        set(WEBVIEW_DLL_SUBDIRECTORY "x86")
    endif()

    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/bin/WebView2Loader.dll"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview_binary-src/build/native/${WEBVIEW_DLL_SUBDIRECTORY}/WebView2Loader.dll" "${CMAKE_BINARY_DIR}/bin"
    )
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/lib/WebView2Loader.lib"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview_binary-src/build/native/${WEBVIEW_DLL_SUBDIRECTORY}/WebView2Loader.dll.lib" "${CMAKE_BINARY_DIR}/lib/WebView2Loader.lib"
    )
    # add_custom_command(
    #     OUTPUT "${CMAKE_BINARY_DIR}/include/WebView2.h"
    #     COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
    #     COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview_binary-src/build/native/include/WebView2.h" "${CMAKE_BINARY_DIR}/include"
    # )
    # add_custom_command(
    #     OUTPUT "${CMAKE_BINARY_DIR}/include/WebView2EnvironmentOptions.h"
    #     COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
    #     COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview_binary-src/build/native/include/WebView2EnvironmentOptions.h" "${CMAKE_BINARY_DIR}/include"
    # )
    add_custom_target(
        webview2-win
        DEPENDS "${CMAKE_BINARY_DIR}/bin/WebView2Loader.dll"
        DEPENDS "${CMAKE_BINARY_DIR}/lib/WebView2Loader.lib"
        # DEPENDS "${CMAKE_BINARY_DIR}/include/WebView2.h"
        # DEPENDS "${CMAKE_BINARY_DIR}/include/WebView2EnvironmentOptions.h"
    )
endif()

add_custom_target(
    webview-prep 
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include" 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/_deps/webview_raw-src/webview.h" "${CMAKE_BINARY_DIR}/include/webview.h"
    COMMAND_EXPAND_LISTS
)

add_library(webview INTERFACE)
target_include_directories(webview INTERFACE "${CMAKE_BINARY_DIR}/include")
add_dependencies(webview webview-prep)

if (WIN32)
    target_include_directories(webview INTERFACE "${CMAKE_BINARY_DIR}/_deps/webview_binary-src/build/native/include")
    target_link_directories(
        webview 
        INTERFACE 
            "${CMAKE_BINARY_DIR}/_deps/webview_binary-src/build/native/${WEBVIEW_DLL_SUBDIRECTORY}"
            "${CMAKE_BINARY_DIR}/lib"
    )
    add_dependencies(webview webview2-win)
endif()

set_target_properties(webview PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:webview,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(
    webview
    INTERFACE
        $<$<PLATFORM_ID:Linux>:
            PkgConfig::webkit2
        >
)