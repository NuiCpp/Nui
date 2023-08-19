include(FetchContent)
FetchContent_Declare(
    libcpppre
    GIT_REPOSITORY https://github.com/cpp-pre/type_traits.git
    GIT_TAG        ffc013750515f9e73342eb2a51efed284826ead3   
)

FetchContent_GetProperties(libcpppre)
if(NOT libcpppre_POPULATED)
    FetchContent_Populate(libcpppre)
endif()

add_library(libcpppre INTERFACE)
target_include_directories(libcpppre INTERFACE ${CMAKE_BINARY_DIR}/_deps/libcpppre-src)