option(NUI_FETCH_INTERVAL_TREE "Fetch interval tree" ON)
set(NUI_INTERVAL_TREE_GIT_REPOSITORY "https://github.com/5cript/interval-tree.git" CACHE STRING "interval tree git repository")
set(NUI_INTERVAL_TREE_GIT_TAG "309b9c725191d4bb1d134f28a8a32ad2f68a8ffa" CACHE STRING "interval tree git tag")

if(NUI_FETCH_INTERVAL_TREE)
    include(FetchContent)
    FetchContent_Declare(
        interval-tree
        GIT_REPOSITORY ${NUI_INTERVAL_TREE_GIT_REPOSITORY}
        GIT_TAG        ${NUI_INTERVAL_TREE_GIT_TAG}    
    )

    FetchContent_MakeAvailable(interval-tree)
endif()