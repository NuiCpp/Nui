add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/src/inline_parser")

if (${NUI_ENABLE_TESTS})
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/test")
endif()