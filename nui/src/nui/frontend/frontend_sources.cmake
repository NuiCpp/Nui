set(NUI_FRONTEND_SOURCES_RELATIVE
    api/fetch.cpp
    api/throttle.cpp
    api/timer.cpp
    attributes/impl/attribute.cpp
    components/dialog.cpp
    dom/dom.cpp
    extensions/make_resizeable.cpp
    filesystem/file_dialog.cpp
    filesystem/file.cpp
    utility/fragment_listener.cpp
    utility/functions.cpp
    utility/stabilize.cpp
    window.cpp
    screen.cpp
    environment_variables.cpp
)

set(NUI_FRONTEND_SOURCES "")

foreach(PATH IN LISTS NUI_FRONTEND_SOURCES_RELATIVE)
    list(APPEND NUI_FRONTEND_SOURCES ${CMAKE_CURRENT_LIST_DIR}/${PATH})
endforeach()