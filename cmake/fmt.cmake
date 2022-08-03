project(fmt-git NONE)

include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        5f774c0aed4af6494b412cd3883e0f09295bd79d    
)

FetchContent_MakeAvailable(fmt)