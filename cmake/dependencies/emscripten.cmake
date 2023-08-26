option(NUI_FETCH_EMSCRIPTEN "Fetch emscripten from git" ON)
set(NUI_EMSCRIPTEN_REPOSITORY "https://github.com/emscripten-core/emsdk.git" CACHE STRING "Emscripten repository")
set(NUI_EMSCRIPTEN_TAG "17f6a2ef92f198f3c9ff30d07664e4090a0ecaf7" CACHE STRING "Emscripten tag")

if (NUI_FETCH_EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        emscripten
        GIT_REPOSITORY ${NUI_EMSCRIPTEN_REPOSITORY}
        GIT_TAG        ${NUI_EMSCRIPTEN_TAG}
    )
    FetchContent_MakeAvailable(emscripten)
endif()