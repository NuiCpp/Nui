function(nui_set_target_output_directories target)
  set_target_properties(${target}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endfunction()

add_library(project-warnings INTERFACE)
include(${CMAKE_SOURCE_DIR}/cmake/compilerwarnings.cmake)
nui_set_project_warnings(project-warnings)