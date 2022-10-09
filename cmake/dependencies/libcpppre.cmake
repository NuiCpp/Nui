project(libcpppre-git NONE)

include(FetchContent)
FetchContent_Declare(
    libcpppre
    GIT_REPOSITORY https://github.com/cpp-pre/type_traits.git
    GIT_TAG        6babbfe9488760f94f5ad63e039807f032049c17    
)

FetchContent_GetProperties(libcpppre)
if(NOT libcpppre_POPULATED)
    FetchContent_Populate(libcpppre)
endif()

add_library(libcpppre INTERFACE)
target_include_directories(libcpppre INTERFACE ${CMAKE_BINARY_DIR}/_deps/libcpppre-src)