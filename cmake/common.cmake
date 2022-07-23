add_library(project-settings INTERFACE)

function(nui_set_target_output_directories target)
  set_target_properties(${target}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endfunction()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (MSYS2_CLANG)
    #find_library(GCC_ATOMIC NAMES atomic PATHS "/mingw64/bin" "/mingw64/lib")
    #target_link_libraries(project-settings INTERFACE ${GCC_ATOMIC})
  else()
    target_link_libraries(project-settings INTERFACE atomic)
  endif()
  #target_link_libraries(project-settings INTERFACE)
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
endif()

add_library(project-warnings INTERFACE)
include(${CMAKE_SOURCE_DIR}/cmake/compilerwarnings.cmake)
nui_set_project_warnings(project-warnings)