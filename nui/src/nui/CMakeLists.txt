if (EMSCRIPTEN)
    add_library(nui-frontend
        STATIC
            frontend/dom/dom.cpp
            frontend/window.cpp
            frontend/utility/functions.cpp
            frontend/event_system/event_context.cpp
    )
    target_include_directories(
        nui-frontend
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/../../include
    )
    target_link_libraries(
        nui-frontend
        PUBLIC
            boost_preprocessor
            libcpppre
        PRIVATE
            project-warnings
    )
    nui_set_target_output_directories(nui-frontend)
else()
    add_library(nui-backend
        STATIC
            backend/window.cpp
    )
    target_include_directories(
        nui-backend
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/../../include
            ${CMAKE_BINARY_DIR}/include
    )
    target_link_libraries(
        nui-backend
        PRIVATE
            project-settings
            project-warnings
            fmt
        PUBLIC
            webview
    )
    if (WIN32)
        target_include_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/include)
        target_link_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/x64)
        target_link_libraries(nui-backend PUBLIC WebView2Loader.dll ole32 shell32 shlwapi user32)
    endif()
    nui_set_target_output_directories(nui-backend)
endif()