project(nlohmann_json-git NONE)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        9dfa7226693012ed5bcf5ab3bc5d8e69d58006ab    
    FIND_PACKAGE_ARGS 
)

FetchContent_MakeAvailable(nlohmann_json)