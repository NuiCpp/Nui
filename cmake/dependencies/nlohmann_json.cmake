option(NUI_FIND_NLOHMANN_JSON "Try find_package to find nlohmann_json" ON)
option(NUI_FETCH_NLOHMANN_JSON "Try fetch nlohmann json" ON)
set(NUI_NLOHMANN_JSON_GIT_REPOSITORY "https://github.com/nlohmann/json.git" CACHE STRING "nlohmann_json git repository")
set(NUI_NLOHMANN_JSON_GIT_TAG "8c391e04fe4195d8be862c97f38cfe10e2a3472e" CACHE STRING "nlohmann_json git tag")

include("${CMAKE_CURRENT_LIST_DIR}/../fetcher.cmake")

nui_fetch_dependency(
    LIBRARY_NAME nlohmann_json
    FIND ${NUI_FIND_NLOHMANN_JSON}
    FETCH ${NUI_FETCH_NLOHMANN_JSON}
    GIT_REPOSITORY ${NUI_NLOHMANN_JSON_GIT_REPOSITORY}
    GIT_TAG ${NUI_NLOHMANN_JSON_GIT_TAG}
)

if (NUI_JSON_DIAGNOSTICS)
    target_compile_definitions(nlohmann_json INTERFACE JSON_DIAGNOSTICS=1)
endif()