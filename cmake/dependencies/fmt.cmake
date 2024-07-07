option(NUI_FIND_FMT "Find fmt first before fetch content" ON)
option(NUI_FETCH_FMT "Try FetchContent for fmt" ON)
set(NUI_FMT_GIT_REPOSITORY "https://github.com/fmtlib/fmt.git" CACHE STRING "fmt git repository")
set(NUI_FMT_GIT_TAG "7a8b54a0ef7ca1791b0a5491b6e0593e1cf2dd5e" CACHE STRING "fmt git tag")

include("${CMAKE_CURRENT_LIST_DIR}/../fetcher.cmake")

nui_fetch_dependency(
    LIBRARY_NAME fmt
    FIND ${NUI_FIND_FMT}
    FETCH ${NUI_FETCH_FMT}
    GIT_REPOSITORY ${NUI_FMT_GIT_REPOSITORY}
    GIT_TAG ${NUI_FMT_GIT_TAG}
)