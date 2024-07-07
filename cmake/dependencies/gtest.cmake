option(NUI_FIND_GTEST "Find gtest" ON)
option(NUI_FETCH_GTEST "Fetch gtest" ON)
set(NUI_GTEST_GIT_REPOSITORY "https://github.com/google/googletest.git" CACHE STRING "gtest git repository")
set(NUI_GTEST_GIT_TAG "beb552fb47e9e8a6ddab20526663c2dddd601ec6" CACHE STRING "gtest git tag")

include("${CMAKE_CURRENT_LIST_DIR}/../fetcher.cmake")

nui_fetch_dependency(
    LIBRARY_NAME GTest
    FIND ${NUI_FIND_GTEST}
    FETCH ${NUI_FETCH_GTEST}
    GIT_REPOSITORY ${NUI_GTEST_GIT_REPOSITORY}
    GIT_TAG ${NUI_GTEST_GIT_TAG}
)