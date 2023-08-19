include(FetchContent)
FetchContent_Declare(
    boost_mp11
    GIT_REPOSITORY https://github.com/boostorg/mp11.git
    GIT_TAG        f6133a9f1f965d89676a33c4a39b3df09373b929    
)

FetchContent_MakeAvailable(boost_mp11)