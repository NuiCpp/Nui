if (EMSCRIPTEN)
    add_library(nui-frontend
        STATIC
            frontend/window.cpp
    )
    target_include_directories(
        nui-frontend
        PUBLIC
            ${CMAKE_SOURCE_DIR}/nui/include
    )
    target_link_libraries(
        nui-frontend
        PRIVATE
            project-warnings
        PUBLIC
            stlab
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
            ${CMAKE_SOURCE_DIR}/nui/include
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
            stlab
    )
    if (WIN32)
        target_include_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/include)
        target_link_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/x64)
        target_link_libraries(nui-backend PUBLIC WebView2Loader.dll ole32 shell32 shlwapi user32)
    endif()
    nui_set_target_output_directories(nui-backend)
endif()