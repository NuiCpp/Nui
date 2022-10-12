if (EMSCRIPTEN)
    add_library(nui-frontend
        STATIC
            frontend/components/dialog.cpp
            frontend/dom/dom.cpp
            frontend/event_system/event_context.cpp
            frontend/utility/functions.cpp
            frontend/window.cpp
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
            mplex
            interval-tree
        PRIVATE
            project-warnings
    )
    nui_set_target_output_directories(nui-frontend)
    target_compile_features(nui-frontend PUBLIC cxx_std_20)
    set_target_properties(nui-frontend PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties(nui-frontend PROPERTIES CXX_EXTENSIONS OFF)
else()
    add_library(nui-backend
        STATIC
            backend/window.cpp
            backend/filesystem/special_paths.cpp
            backend/rpc_hub.cpp
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
        PUBLIC
            fmt
            webview
    )
    if (WIN32)
        target_include_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/include)
        target_link_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/x64)
        target_link_libraries(nui-backend PUBLIC WebView2Loader.dll ole32 shell32 shlwapi user32)
    endif()
    nui_set_target_output_directories(nui-backend)
    target_compile_features(nui-backend PUBLIC cxx_std_20)
    set_target_properties(nui-backend PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties(nui-backend PROPERTIES CXX_EXTENSIONS OFF)
endif()