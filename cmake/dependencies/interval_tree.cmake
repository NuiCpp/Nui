project(interval-tree-git NONE)

include(FetchContent)
FetchContent_Declare(
    interval-tree
    GIT_REPOSITORY https://github.com/5cript/interval-tree.git
    GIT_TAG        309b9c725191d4bb1d134f28a8a32ad2f68a8ffa    
)

FetchContent_MakeAvailable(interval-tree)