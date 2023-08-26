if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(
        webkit2 REQUIRED webkit2gtk-4.0
        IMPORTED_TARGET
    )
endif()

option(NUI_FETCH_WEBVIEW "Fetch webview" ON)
set(NUI_WEBVIEW_REPOSITORY "https://github.com/5cript/webview.git" CACHE STRING "Webview repository")
set(NUI_WEBVIEW_TAG "5002fb32b3c6981e5aae9bcee6f07a7dc3ce3dac" CACHE STRING "Webview tag")

if (NUI_FETCH_WEBVIEW)
    include(FetchContent)
    FetchContent_Declare(
        webview_raw
        GIT_REPOSITORY ${NUI_WEBVIEW_REPOSITORY}
        GIT_TAG        ${NUI_WEBVIEW_TAG}
    )

    FetchContent_MakeAvailable(webview_raw)
endif()

if (WIN32)
    option(NUI_FETCH_WEBVIEW_BINARY "Fetch webview binary" ON)
    set(NUI_WEBVIEW_BINARY_URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2" CACHE STRING "Webview binary url")

    if (NUI_FETCH_WEBVIEW_BINARY)
        include(FetchContent)
        FetchContent_Declare(
            webview_binary
            URL ${NUI_WEBVIEW_BINARY_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
        FetchContent_GetProperties(webview_binary)
        if (webview_binary_POPULATED)
        else()
            FetchContent_Populate(webview_binary)
        endif()
    endif()

    option(NUI_FETCH_BINARYEN "Fetch binaryen" ON)
    set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_112/binaryen-version_112-x86_64-windows.tar.gz" CACHE STRING "Binaryen url")

    if (NUI_FETCH_BINARYEN)
        include(FetchContent)
        FetchContent_Declare(
            binaryen
            URL ${NUI_BINARYEN_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
        FetchContent_GetProperties(binaryen)
        if (binaryen_POPULATED)
        else()
            FetchContent_Populate(binaryen)
        endif()
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