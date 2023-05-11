project(libcpppre-git NONE)

include(FetchContent)
FetchContent_Declare(
    libcpppre
    GIT_REPOSITORY https://github.com/5cript/type_traits.git
    GIT_TAG        b5973e734c51c9708e10e3a0623545c896cea78b   
)

FetchContent_GetProperties(libcpppre)
if(NOT libcpppre_POPULATED)
    FetchContent_Populate(libcpppre)
endif()

add_library(libcpppre INTERFACE)
target_include_directories(libcpppre INTERFACE ${CMAKE_BINARY_DIR}/_deps/libcpppre-src)