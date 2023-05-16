include(GoogleTest)
include(../../src/nui/frontend/frontend_sources.cmake)

add_library(nui-frontend-mocked STATIC ${NUI_FRONTEND_SOURCES})

target_include_directories(nui-frontend-mocked PUBLIC emscripten_mock ${CMAKE_CURRENT_LIST_DIR}/../../include)

find_package(Boost REQUIRED)

target_link_libraries(nui-frontend-mocked PUBLIC
    libcpppre
    mplex
    interval-tree
    Boost::boost
)
target_compile_definitions(nui-frontend-mocked PRIVATE __EMSCRIPTEN__)
target_compile_features(nui-frontend-mocked PUBLIC cxx_std_23)
set_target_properties(nui-frontend-mocked PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(nui-frontend-mocked PROPERTIES CXX_EXTENSIONS ON)

add_executable(nui-tests
    tests.cpp
    engine/value.cpp
    engine/global_object.cpp
    engine/warn.cpp
    engine/document.cpp
)
target_link_libraries(nui-tests PRIVATE 
    nui-frontend-mocked 
    gtest 
)
gtest_discover_tests(nui-tests)
