include(FetchContent)
FetchContent_Declare(
    boost_describe 
    GIT_REPOSITORY https://github.com/boostorg/describe.git
    GIT_TAG        f6bf0c4f1e157ead471ae17cb39c3b667815a1b2    
)

FetchContent_MakeAvailable(boost_describe)