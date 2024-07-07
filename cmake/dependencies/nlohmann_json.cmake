option(NUI_FETCH_NLOHMANN_JSON "Fetch nlohmann_json" ON)
set(NUI_NLOHMANN_JSON_GIT_REPOSITORY "https://github.com/nlohmann/json.git" CACHE STRING "nlohmann_json git repository")
set(NUI_NLOHMANN_JSON_GIT_TAG "8c391e04fe4195d8be862c97f38cfe10e2a3472e" CACHE STRING "nlohmann_json git tag")

if(NUI_FETCH_NLOHMANN_JSON)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY ${NUI_NLOHMANN_JSON_GIT_REPOSITORY}
        GIT_TAG        ${NUI_NLOHMANN_JSON_GIT_TAG}
    )

    FetchContent_MakeAvailable(nlohmann_json)
endif()