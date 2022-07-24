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

if (${NUI_USE_EXTERNAL_EMSCRIPTEN})
    set(EMCMAKE "emcmake")
    set(EMMAKE "emmake")
else()
    set(EMCMAKE "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcmake")
    set(EMMAKE "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emmake")
    list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten")
endif()

function(nui_add_emscripten_target target sourceDir cmakeOptions)
    if (sourceDir STREQUAL "")
        get_target_property(sourceDir ${target} SOURCE_DIR)
    endif()
    ExternalProject_Add(
        "${target}-emscripten"
        SOURCE_DIR "${sourceDir}"
        CONFIGURE_COMMAND ${EMCMAKE} cmake ${cmakeOptions} ${sourceDir}
        BUILD_COMMAND ${EMMAKE} make ${target}
        BINARY_DIR "${CMAKE_BINARY_DIR}/modules_build"
        INSTALL_COMMAND ""
        INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/modules ${CMAKE_BINARY_DIR}/modules
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${CMAKE_BINARY_DIR}/modules_build/bin/${target}.js 
            ${CMAKE_BINARY_DIR}/modules_build/bin/${target}.wasm
            ${CMAKE_BINARY_DIR}/modules
    )
endfunction()
