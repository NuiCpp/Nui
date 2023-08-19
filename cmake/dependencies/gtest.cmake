include(FetchContent)
FetchContent_Declare(
    gtest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        cead3d57c93ff8c4e5c1bbae57a5c0b0b0f6e168    
    FIND_PACKAGE_ARGS NAMES gtest
)

FetchContent_MakeAvailable(gtest)