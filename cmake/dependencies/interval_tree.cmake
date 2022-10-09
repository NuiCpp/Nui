project(interval-tree-git NONE)

include(FetchContent)
FetchContent_Declare(
    interval-tree
    GIT_REPOSITORY https://github.com/5cript/interval-tree.git
    GIT_TAG        14f1092c6a12ed03accc9416971f1e6204acf259    
)

FetchContent_MakeAvailable(interval-tree)