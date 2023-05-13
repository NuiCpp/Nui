include(./frontend_sources.cmake)
add_library(nui-frontend STATIC ${NUI_FRONTEND_SOURCES})
add_library(Nui::frontend ALIAS nui-frontend)
target_include_directories(
    nui-frontend
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../../../include
)
target_link_libraries(
    nui-frontend
    PUBLIC
        boost_preprocessor
        libcpppre
        mplex
        interval-tree
        boost_describe
        boost_mp11
)
nui_set_project_warnings(nui-frontend)
nui_set_target_output_directories(nui-frontend)
target_compile_features(nui-frontend PUBLIC cxx_std_23)
set_target_properties(nui-frontend PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(nui-frontend PROPERTIES CXX_EXTENSIONS ON)
