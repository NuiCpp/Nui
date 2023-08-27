option(NUI_FETCH_GTEST "Fetch gtest" ON)
set(NUI_GTEST_GIT_REPOSITORY "https://github.com/google/googletest.git" CACHE STRING "gtest git repository")
set(NUI_GTEST_GIT_TAG "cead3d57c93ff8c4e5c1bbae57a5c0b0b0f6e168" CACHE STRING "gtest git tag")

if(NUI_FETCH_GTEST)
    include(FetchContent)
    FetchContent_Declare(
        gtest
        GIT_REPOSITORY ${NUI_GTEST_GIT_REPOSITORY}
        GIT_TAG        ${NUI_GTEST_GIT_TAG}
        FIND_PACKAGE_ARGS NAMES gtest
    )

    FetchContent_MakeAvailable(gtest)
endif()