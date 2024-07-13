add_library(nui-events STATIC event_context.cpp)
add_library(Nui::events ALIAS nui-events)
target_include_directories(
    nui-events
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/../../../include"
)
target_link_libraries(
    nui-events
    PUBLIC
        boost_preprocessor
        interval-tree
)
nui_set_project_warnings(nui-events)
nui_set_target_output_directories(nui-events)
target_compile_features(nui-events PUBLIC cxx_std_20)
set_target_properties(nui-events PROPERTIES CXX_STANDARD_REQUIRED OFF)
set_target_properties(nui-events PROPERTIES CXX_EXTENSIONS OFF)
