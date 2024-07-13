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
    nui-events
)
target_compile_definitions(nui-frontend-mocked PRIVATE __EMSCRIPTEN__)
target_compile_features(nui-frontend-mocked PUBLIC cxx_std_20)
set_target_properties(nui-frontend-mocked PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(nui-frontend-mocked PROPERTIES CXX_EXTENSIONS ON)

add_executable(nui-tests
    tests.cpp
    engine/value.cpp
    engine/global_object.cpp
    engine/warn.cpp
    engine/document.cpp
    engine/object.cpp
    engine/array.cpp
)
target_link_libraries(nui-tests PRIVATE
    nui-frontend-mocked
    gtest
)
gtest_discover_tests(nui-tests)

set_target_properties(nui-tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# If msys2, copy dynamic libraries to executable directory, visual studio does this automatically.
# And there is no need on linux.
if (DEFINED ENV{MSYSTEM})
    add_custom_command(TARGET nui-tests POST_BUILD
        COMMAND bash -c "ldd $<TARGET_FILE:nui-tests>" | "grep" "clang" | awk "NF == 4 { system(\"${CMAKE_COMMAND} -E copy \" \$3 \" $<TARGET_FILE_DIR:nui-tests>\") }"
        VERBATIM
    )
endif()