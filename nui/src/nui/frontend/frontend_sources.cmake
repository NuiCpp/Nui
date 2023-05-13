set(NUI_FRONTEND_SOURCES_RELATIVE
    components/dialog.cpp
    dom/dom.cpp
    event_system/event_context.cpp
    filesystem/file_dialog.cpp
    filesystem/file.cpp
    utility/fragment_listener.cpp
    utility/functions.cpp
    window.cpp
    api/fetch.cpp
    api/throttle.cpp
    api/timer.cpp
    screen.cpp
    environment_variables.cpp
)

set(NUI_FRONTEND_SOURCES "")

foreach(PATH IN LISTS NUI_FRONTEND_SOURCES_RELATIVE)
    list(APPEND NUI_FRONTEND_SOURCES ${CMAKE_CURRENT_LIST_DIR}/${PATH})
endforeach()