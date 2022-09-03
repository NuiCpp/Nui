project(boost_preprocessor-git NONE)

include(FetchContent)
FetchContent_Declare(
    boost_preprocessor
    GIT_REPOSITORY https://github.com/boostorg/preprocessor.git
    GIT_TAG        667e87b3392db338a919cbe0213979713aca52e3    
)

FetchContent_MakeAvailable(boost_preprocessor)