option(NUI_FETCH_BOOST_PREPROCESSOR "Automatically pull the boost preprocessor repository" ON)
set(NUI_BOOST_PREPROCESSOR_GIT_REPOSITORY "https://github.com/boostorg/preprocessor.git" CACHE STRING "The URL from which to clone the boost preprocessor repository")
set(NUI_BOOST_PREPROCESSOR_GIT_TAG "667e87b3392db338a919cbe0213979713aca52e3" CACHE STRING "The git tag or commit hash to checkout from the boost preprocessor repository")

if(NUI_FETCH_BOOST_PREPROCESSOR)
    include(FetchContent)
    FetchContent_Declare(
        boost_preprocessor
        GIT_REPOSITORY ${NUI_BOOST_PREPROCESSOR_GIT_REPOSITORY}
        GIT_TAG        ${NUI_BOOST_PREPROCESSOR_GIT_TAG}    
    )

    FetchContent_MakeAvailable(boost_preprocessor)
endif()