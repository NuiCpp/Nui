option(NUI_FETCH_BOOST_MP11 "Fetch boost mp11" ON)
set(NUI_BOOST_MP11_GIT_REPOSITORY "https://github.com/boostorg/mp11.git" CACHE STRING "boost mp11 git repository")
set(NUI_BOOST_MP11_GIT_TAG "f6133a9f1f965d89676a33c4a39b3df09373b929" CACHE STRING "boost mp11 git tag")

if(NUI_FETCH_BOOST_MP11)
    include(FetchContent)
    FetchContent_Declare(
        boost_mp11
        GIT_REPOSITORY ${NUI_BOOST_MP11_GIT_REPOSITORY}
        GIT_TAG        ${NUI_BOOST_MP11_GIT_TAG}
    )

    FetchContent_MakeAvailable(boost_mp11)
endif()