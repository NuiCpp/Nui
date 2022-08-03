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
    string(REPLACE "-" "_" target_normalized ${target})
    ExternalProject_Add(
        "${target}-emscripten"
        SOURCE_DIR "${sourceDir}"
        CONFIGURE_COMMAND ${EMCMAKE} cmake -DCMAKE_CXX_STANDARD=20 ${cmakeOptions} ${sourceDir}
        BUILD_COMMAND ${EMMAKE} make ${target}
        COMMAND ${CMAKE_BINARY_DIR}/tools/bin2hpp/bin2hpp ${CMAKE_BINARY_DIR}/module_build/bin/${target}.js ${CMAKE_BINARY_DIR}/include/${target_normalized}.hpp ${target_normalized}
        BINARY_DIR "${CMAKE_BINARY_DIR}/module_build"
        BUILD_ALWAYS 1
        INSTALL_COMMAND ""
    )
    add_dependencies(${target} ${target}-emscripten)
    add_dependencies(${target}-emscripten bin2hpp)
endfunction()
