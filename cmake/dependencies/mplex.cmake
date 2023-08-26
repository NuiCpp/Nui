option(NUI_FETCH_MPLEX "Fetch 5cript/mplex" ON)
set(NUI_MPLEX_GIT_REPOSITORY "https://github.com/5cript/mplex.git" CACHE STRING "5cript/mplex git repository")
set(NUI_MPLEX_GIT_TAG "a3f9ffcec4c1101b68520ebc598f87ca979ba252" CACHE STRING "5cript/mplex git tag")

if(NUI_FETCH_MPLEX)
    include(FetchContent)
    FetchContent_Declare(
        mplex
        GIT_REPOSITORY ${NUI_MPLEX_GIT_REPOSITORY}
        GIT_TAG        ${NUI_MPLEX_GIT_TAG}
    )

    FetchContent_MakeAvailable(mplex)
endif()