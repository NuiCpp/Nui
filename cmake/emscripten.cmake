project(emscripten-git NONE)

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
    emscripten
    GIT_REPOSITORY https://github.com/5cript/emsdk
    GIT_TAG        4ab2df239753b9cc22dcf987140319524e7e2a60 # Main branch after msys2 fix    
)

FetchContent_MakeAvailable(emscripten)

add_custom_target(
    emscripten_setup
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/emscripten-src"
    COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" install latest --build=Release
    COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcc --generate-config"
)

list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten")

function(nui_add_emscripten_target target)
    set(externalProjectName "${target}-emscripten")
    message("target ${target}")
    get_target_property(sourceDir ${target} SOURCE_DIR)
    message("sourceDir ${sourceDir}")
    ExternalProject_Add(
        "${target}_emscripten"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modules_${target}"
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/modules_${target}"
        SOURCE_DIR "${sourceDir}"
        CONFIGURE_COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcmake cmake ${sourceDir}
        BUILD_COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emmake make
    )
endfunction()