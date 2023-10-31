add_library(nui-backend
    STATIC
        window.cpp
        filesystem/special_paths.cpp
        filesystem/file_dialog.cpp
        filesystem/file_dialog_options.cpp
        rpc_hub.cpp
        rpc_addons/fetch.cpp
        rpc_addons/file.cpp
        rpc_addons/throttle.cpp
        rpc_addons/timer.cpp
        rpc_addons/screen.cpp
        rpc_addons/environment_variables.cpp
)
add_library(Nui::backend ALIAS nui-backend)
target_include_directories(
    nui-backend
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../../../include
        ${CMAKE_CURRENT_LIST_DIR}/../../../windows
        ${CMAKE_BINARY_DIR}/include
)
nui_set_project_warnings(nui-backend)

find_package(Boost 1.80.0 REQUIRED COMPONENTS system)

target_link_libraries(
    nui-backend
    PRIVATE
        project-settings
    PUBLIC
        fmt
        webview
        portable_file_dialogs
        nlohmann_json
        roar
        Boost::boost
)

if (APPLE)
    target_link_libraries(nui-backend PUBLIC
        "-framework CoreGraphics"
        "-framework IOKit"
        "-framework Foundation"
    )
endif()

if (WIN32)
    target_include_directories(nui-backend PUBLIC ${CMAKE_BINARY_DIR}/libs/webview2/build/native/include)
    target_link_libraries(nui-backend PUBLIC WebView2Loader.dll ole32 shell32 shlwapi user32)
    target_sources(nui-backend
        PRIVATE
            screen_win.cpp
            environment_variables_win.cpp
    )
elseif(APPLE)
    target_sources(nui-backend
        PRIVATE
            screen_mac.cpp
            environment_variables_nix.cpp
    )
else()
    target_sources(nui-backend
        PRIVATE
            screen_nix.cpp
            environment_variables_nix.cpp
    )
endif()
nui_set_target_output_directories(nui-backend)
target_compile_features(nui-backend PUBLIC cxx_std_20)
set_target_properties(nui-backend PROPERTIES CXX_STANDARD_REQUIRED OFF)
set_target_properties(nui-backend PROPERTIES CXX_EXTENSIONS OFF)
