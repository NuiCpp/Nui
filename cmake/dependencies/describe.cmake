option(NUI_FETCH_BOOST_DESCRIBE "Fetch boost describe" ON)
set(NUI_BOOST_DESCRIBE_GIT_REPOSITORY "https://github.com/boostorg/describe.git" CACHE STRING "boost describe git repository")
set(NUI_BOOST_DESCRIBE_GIT_TAG "f6bf0c4f1e157ead471ae17cb39c3b667815a1b2" CACHE STRING "boost describe git tag")

if(NUI_FETCH_BOOST_DESCRIBE)
    include(FetchContent)
    FetchContent_Declare(
        boost_describe 
        GIT_REPOSITORY ${NUI_BOOST_DESCRIBE_GIT_REPOSITORY}
        GIT_TAG        ${NUI_BOOST_DESCRIBE_GIT_TAG}
    )

    FetchContent_MakeAvailable(boost_describe)
endif()