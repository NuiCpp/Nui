option(NUI_FETCH_FMT "Fetch fmt" ON)
set(NUI_FMT_GIT_REPOSITORY "https://github.com/fmtlib/fmt.git" CACHE STRING "fmt git repository")
set(NUI_FMT_GIT_TAG "4e8640ed90ac8751d4a8ca500b893cc8c4bb9668" CACHE STRING "fmt git tag")

if (NUI_FETCH_FMT)
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY ${NUI_FMT_GIT_REPOSITORY}
        GIT_TAG        ${NUI_FMT_GIT_TAG}
    )

    FetchContent_MakeAvailable(fmt)
endif()