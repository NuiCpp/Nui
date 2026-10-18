# CEF engine part of nui-backend, included from the backend CMakeLists.txt.

# Code of the browser process and of the helper: the CefApp, the renderer side and the message format.
add_library(nui-cef-common STATIC
    "${CMAKE_CURRENT_LIST_DIR}/common/app.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/common/messages.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/common/render_process.cpp"
)
target_link_libraries(nui-cef-common PUBLIC nui-cef-headers libcef_dll_wrapper)
if (NOT APPLE)
    target_link_libraries(nui-cef-common PUBLIC nui-libcef)
endif()
set_target_properties(nui-cef-common PROPERTIES POSITION_INDEPENDENT_CODE ON)

# One helper for every CEF child process. It does not link the application, so the application's main never runs in a
# child process.
add_executable(nui-cef-helper "${CMAKE_CURRENT_LIST_DIR}/helper/helper_main.cpp")
target_link_libraries(nui-cef-helper PRIVATE nui-cef-common)
if (APPLE)
    set_target_properties(nui-cef-helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/nui-cef$<0:>")
else()
    # Next to the links to libcef, where the engine looks for it.
    set_target_properties(nui-cef-helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${NUI_CEF_RUNTIME_DIR}$<0:>")
endif()

target_sources(nui-backend
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/client.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/engine.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/gate.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/instance_slot.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/scheme_handler.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/screen.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/window.cpp"
)
target_link_libraries(nui-backend PRIVATE nui-cef-common)
target_compile_definitions(nui-backend PUBLIC NUI_ENGINE_CEF)
add_dependencies(nui-backend nui-cef-helper)

if (APPLE)
    target_sources(nui-backend PRIVATE "${CMAKE_CURRENT_LIST_DIR}/mac/application_mac.mm")
    target_link_libraries(nui-backend PUBLIC "-framework AppKit")
else()
    target_sources(nui-backend PRIVATE "${CMAKE_CURRENT_LIST_DIR}/sandbox_linux.cpp")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NUI_CEF_GLIB REQUIRED IMPORTED_TARGET glib-2.0)
    target_link_libraries(nui-backend PRIVATE PkgConfig::NUI_CEF_GLIB ${CMAKE_DL_LIBS})
endif()
