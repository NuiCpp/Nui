option(NUI_FETCH_FMT "Fetch fmt" ON)
set(NUI_FMT_GIT_REPOSITORY "https://github.com/fmtlib/fmt.git" CACHE STRING "fmt git repository")
set(NUI_FMT_GIT_TAG "5f774c0aed4af6494b412cd3883e0f09295bd79d" CACHE STRING "fmt git tag")

if (NUI_FETCH_FMT)
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY ${NUI_FMT_GIT_REPOSITORY}
        GIT_TAG        ${NUI_FMT_GIT_TAG}
    )

    FetchContent_MakeAvailable(fmt)
endif()