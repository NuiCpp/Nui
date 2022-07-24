project(emscripten-git NONE)

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
    emscripten
    GIT_REPOSITORY https://github.com/5cript/emsdk
    GIT_TAG        4ab2df239753b9cc22dcf987140319524e7e2a60 # Main branch after msys2 fix    
)

FetchContent_MakeAvailable(emscripten)

if(UNIX)
    add_custom_target(
        emscripten_setup
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/emscripten-src"
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" install latest --build=Release
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" activate latest
        COMMAND_EXPAND_LISTS
    )
else()
    add_custom_target(
        emscripten_setup
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/emscripten-src"
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" install latest --build=Release
        #FIXME: properly generate config / get activate to work.
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcc --generate-config"
        COMMAND_EXPAND_LISTS
    )
endif()

list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten")

function(nui_add_emscripten_target target sourceDir cmakeOptions)
    set(externalProjectName "${target}-emscripten")
    message("target ${target}")
    if (sourceDir STREQUAL "")
        get_target_property(sourceDir ${target} SOURCE_DIR)
    endif()
    message("sourceDir ${sourceDir}")
    ExternalProject_Add(
        "${target}_emscripten"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/modules_${target}"
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/modules_${target}"
        SOURCE_DIR "${sourceDir}"
        CONFIGURE_COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcmake cmake ${cmakeOptions} ${sourceDir}
        BUILD_COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emmake make ${target}
        INSTALL_COMMAND ""
    )
endfunction()
