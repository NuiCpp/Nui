project(range-v3-git NONE)

include(FetchContent)
FetchContent_Declare(
    range-v3
    GIT_REPOSITORY https://github.com/ericniebler/range-v3.git
    GIT_TAG        689b4f3da769fb21dd7acf62550a038242d832e5    
)

FetchContent_MakeAvailable(range-v3)