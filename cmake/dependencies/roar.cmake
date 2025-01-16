option(NUI_FETCH_ROAR "Fetch roar" ON)
set(NUI_ROAR_REPOSITORY "https://github.com/5cript/roar.git" CACHE STRING "roar repository")
set(NUI_ROAR_TAG "ed28f3c14a47d75c57464dbcf29e60a59dcfd534" CACHE STRING "roar tag")

if(NUI_FETCH_ROAR)
    include(FetchContent)
    FetchContent_Declare(
        roar
        GIT_REPOSITORY ${NUI_ROAR_REPOSITORY}
        GIT_TAG        ${NUI_ROAR_TAG}
    )

    FetchContent_MakeAvailable(roar)
endif()