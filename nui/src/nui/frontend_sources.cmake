set(NUI_FRONTEND_SOURCES_RELATIVE
    frontend/components/dialog.cpp
    frontend/dom/dom.cpp
    frontend/event_system/event_context.cpp
    frontend/filesystem/file_dialog.cpp
    frontend/filesystem/file.cpp
    frontend/utility/fragment_listener.cpp
    frontend/utility/functions.cpp
    frontend/window.cpp
    frontend/api/fetch.cpp
    frontend/api/throttle.cpp
    frontend/api/timer.cpp
    frontend/screen.cpp
    frontend/environment_variables.cpp
)

set(NUI_FRONTEND_SOURCES "")

foreach(PATH IN LISTS NUI_FRONTEND_SOURCES_RELATIVE)
    list(APPEND NUI_FRONTEND_SOURCES ${CMAKE_CURRENT_LIST_DIR}/${PATH})
endforeach()