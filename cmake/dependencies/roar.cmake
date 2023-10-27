option(NUI_FETCH_ROAR "Fetch roar" ON)
set(NUI_ROAR_REPOSITORY "https://github.com/5cript/roar.git" CACHE STRING "roar repository")
set(NUI_ROAR_TAG "9e60320c7d60f91feb2bc13948340a97dd8ee190" CACHE STRING "roar tag")

if(NUI_FETCH_ROAR)
    include(FetchContent)
    FetchContent_Declare(
        roar
        GIT_REPOSITORY ${NUI_ROAR_REPOSITORY}
        GIT_TAG        ${NUI_ROAR_TAG}
    )

    FetchContent_MakeAvailable(roar)
endif()