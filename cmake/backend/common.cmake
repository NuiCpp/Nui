add_library(project-settings INTERFACE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
else()
    target_link_libraries(project-settings INTERFACE atomic)
endif()