if (APPLE)
elseif (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(
        webkit2 REQUIRED webkit2gtk-4.1 webkit2gtk-4.0 webkit2gtk
        IMPORTED_TARGET
    )
endif()

option(NUI_FETCH_WEBVIEW "Fetch webview" ON)
set(NUI_WEBVIEW_REPOSITORY "https://github.com/5cript/webview.git" CACHE STRING "Webview repository")
set(NUI_WEBVIEW_TAG "c962bbe343c82d58ecfdfb2942d49cc2c1410705" CACHE STRING "Webview tag")

if (NUI_FETCH_WEBVIEW)
    include(FetchContent)
    if (CMAKE_VERSION VERSION_LESS "3.28.0")
        FetchContent_Declare(
            webview_raw
            GIT_REPOSITORY ${NUI_WEBVIEW_REPOSITORY}
            GIT_TAG        ${NUI_WEBVIEW_TAG}
        )
    else()
        FetchContent_Declare(
            webview_raw
            EXCLUDE_FROM_ALL
            GIT_REPOSITORY ${NUI_WEBVIEW_REPOSITORY}
            GIT_TAG        ${NUI_WEBVIEW_TAG}
        )
    endif()
    FetchContent_GetProperties(webview_raw)
    if (webview_raw_POPULATED)
    else()
        FetchContent_Populate(webview_raw)
    endif()

    add_library(webview-iface INTERFACE)

    add_custom_target(
        webview-nui-prep
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/_deps/webview_raw-src/webview.h" "${CMAKE_BINARY_DIR}/include/webview.h"
        COMMAND_EXPAND_LISTS
    )

    target_include_directories(webview-iface INTERFACE "${CMAKE_BINARY_DIR}/include")
    add_dependencies(webview-iface webview-nui-prep)

    set_target_properties(webview-iface PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:webview-iface,INTERFACE_INCLUDE_DIRECTORIES>)

    target_link_libraries(
        webview-iface
        INTERFACE
            $<$<PLATFORM_ID:Linux>:
                PkgConfig::webkit2
            >
    )
    target_link_libraries(
        webview-iface
        INTERFACE
            $<$<PLATFORM_ID:Windows>:
                Version.lib
                WebView2Loader.lib
            >
    )
    target_link_libraries(
        webview-iface
        INTERFACE
            $<$<PLATFORM_ID:Darwin>:
                "-framework WebKit"
            >
    )

    if (WIN32)
        option(NUI_FETCH_WEBVIEW_BINARY "Fetch webview binary" ON)
        set(NUI_WEBVIEW_BINARY_URL "http://www.nuget.org/api/v2/package/Microsoft.Web.WebView2" CACHE STRING "Webview binary url")

        if (NUI_FETCH_WEBVIEW_BINARY)
            include(FetchContent)
            if (CMAKE_VERSION VERSION_LESS "3.24")
                FetchContent_Declare(
                    webview-binary-nui
                    URL ${NUI_WEBVIEW_BINARY_URL}
                )
            else()
                FetchContent_Declare(
                    webview-binary-nui
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    URL ${NUI_WEBVIEW_BINARY_URL}
                )
            endif()
            FetchContent_MakeAvailable(webview-binary-nui)

            if (CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(WEBVIEW_DLL_SUBDIRECTORY "x64")
            else()
                set(WEBVIEW_DLL_SUBDIRECTORY "x86")
            endif()

            target_include_directories(webview-iface INTERFACE "${CMAKE_BINARY_DIR}/_deps/webview-binary-nui-src/build/native/include")
            target_link_directories(
                webview-iface
                INTERFACE
                    "${CMAKE_BINARY_DIR}/_deps/webview-binary-nui-src/build/native/${WEBVIEW_DLL_SUBDIRECTORY}"
                    "${CMAKE_BINARY_DIR}/lib"
            )
            add_dependencies(webview-iface webview2-win-nui)

            add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/bin/WebView2Loader.dll"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview-binary-nui-src/build/native/${WEBVIEW_DLL_SUBDIRECTORY}/WebView2Loader.dll" "${CMAKE_BINARY_DIR}/bin"
            )
            add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/lib/WebView2Loader.lib"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/lib"
                COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/_deps/webview-binary-nui-src/build/native/${WEBVIEW_DLL_SUBDIRECTORY}/WebView2Loader.dll.lib" "${CMAKE_BINARY_DIR}/lib/WebView2Loader.lib"
            )
            add_custom_command(
                OUTPUT "${CMAKE_BINARY_DIR}/include/webview2_iids.h"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/include"
                COMMAND $<TARGET_FILE:webview-uuid> "${CMAKE_BINARY_DIR}/_deps/webview-binary-nui-src/build/native/include/WebView2.h" "${CMAKE_BINARY_DIR}/include/webview2_iids.h"
            )
            add_custom_target(
                webview2-win-nui
                DEPENDS "${CMAKE_BINARY_DIR}/bin/WebView2Loader.dll"
                DEPENDS "${CMAKE_BINARY_DIR}/lib/WebView2Loader.lib"
                DEPENDS "${CMAKE_BINARY_DIR}/include/webview2_iids.h"
            )
        endif()
    endif()
endif()