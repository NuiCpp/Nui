option(NUI_FETCH_EMSCRIPTEN "Fetch emscripten from git" ON)
set(NUI_EMSCRIPTEN_REPOSITORY "https://github.com/emscripten-core/emsdk.git" CACHE STRING "Emscripten repository")
set(NUI_EMSCRIPTEN_TAG "4.0.13" CACHE STRING "Emscripten tag")

if (NUI_FETCH_EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        emscripten
        GIT_REPOSITORY ${NUI_EMSCRIPTEN_REPOSITORY}
        GIT_TAG        ${NUI_EMSCRIPTEN_TAG}
    )
    FetchContent_MakeAvailable(emscripten)
endif()