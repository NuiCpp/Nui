option(NUI_FETCH_ROAR "Fetch roar" ON)
set(NUI_ROAR_REPOSITORY "https://github.com/5cript/roar.git" CACHE STRING "roar repository")
set(NUI_ROAR_TAG "230e4b92f3212f0d77d1b77b50d0efa85d257be8" CACHE STRING "roar tag")

if(NUI_FETCH_ROAR)
    include(FetchContent)
    FetchContent_Declare(
        roar
        GIT_REPOSITORY ${NUI_ROAR_REPOSITORY}
        GIT_TAG        ${NUI_ROAR_TAG}
    )

    FetchContent_MakeAvailable(roar)
endif()