include(GoogleTest)
include(../../src/nui/frontend_sources.cmake)

add_library(nui-frontend-mocked STATIC ${NUI_FRONTEND_SOURCES})

target_include_directories(nui-frontend-mocked PUBLIC mocks ${CMAKE_CURRENT_LIST_DIR}/../../include)

target_link_libraries(nui-frontend-mocked PUBLIC
    boost_preprocessor
    libcpppre
    mplex
    interval-tree
    boost_describe
    boost_mp11
)

add_executable(nui-tests
    tests.cpp
)
target_link_libraries(nui-tests PRIVATE 
    nui-frontend-mocked 
    gtest 
)
gtest_discover_tests(nui-tests)
