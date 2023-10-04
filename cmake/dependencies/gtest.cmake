option(NUI_FETCH_GTEST "Fetch gtest" ON)
set(NUI_GTEST_GIT_REPOSITORY "https://github.com/google/googletest.git" CACHE STRING "gtest git repository")
set(NUI_GTEST_GIT_TAG "beb552fb47e9e8a6ddab20526663c2dddd601ec6" CACHE STRING "gtest git tag")

if(NUI_FETCH_GTEST)
    include(FetchContent)
    FetchContent_Declare(
        gtest
        GIT_REPOSITORY ${NUI_GTEST_GIT_REPOSITORY}
        GIT_TAG        ${NUI_GTEST_GIT_TAG}
    )

    FetchContent_MakeAvailable(gtest)
endif()