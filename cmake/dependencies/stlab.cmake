project(stlab-git NONE)

include(FetchContent)
FetchContent_Declare(
    stlab
    GIT_REPOSITORY https://github.com/stlab/libraries.git
    GIT_TAG        79c8781660b7da0eb08aa7e1c5355c250a5f66e9    
)

FetchContent_GetProperties(stlab)
if(NOT stlab_POPULATED)
    FetchContent_Populate(stlab)
endif()

add_library(stlab INTERFACE)
target_include_directories(stlab INTERFACE ${CMAKE_BINARY_DIR}/_deps/stlab-src)
