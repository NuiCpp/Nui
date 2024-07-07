option(NUI_FETCH_LIBCPPPRE "Fetch libcpppre" ON)
set(NUI_LIBCPPPRE_GIT_REPOSITORY "https://github.com/cpp-pre/type_traits.git" CACHE STRING "libcpppre git repository")
set(NUI_LIBCPPPRE_GIT_TAG "ffc013750515f9e73342eb2a51efed284826ead3" CACHE STRING "libcpppre git tag")

if(NUI_FETCH_LIBCPPPRE)
    include(FetchContent)
    FetchContent_Declare(
        libcpppre
        GIT_REPOSITORY https://github.com/cpp-pre/type_traits.git
        GIT_TAG        ffc013750515f9e73342eb2a51efed284826ead3   
    )

    FetchContent_MakeAvailable(libcpppre)

    add_library(libcpppre INTERFACE)
    target_include_directories(libcpppre INTERFACE ${CMAKE_BINARY_DIR}/_deps/libcpppre-src)
endif()