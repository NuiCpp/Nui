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

function(nui_add_emscripten_target)
    if (${NUI_USE_EXTERNAL_EMSCRIPTEN})
        set(EMCMAKE "emcmake")
        set(EMMAKE "emmake")
    else()
        set(EMCMAKE "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcmake")
        set(EMMAKE "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emmake")
        list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten")
    endif()

    cmake_parse_arguments(
        NUI_ADD_EMSCRIPTEN_TARGET_ARGS
        ""
        "TARGET;PREJS;SOURCE_DIR"
        "CMAKE_OPTIONS;MAKE_OPTIONS"
        ${ARGN}
    )    

    if (NOT NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target to create a frontend pendant of.")
    endif()

    if (NOT NUI_ADD_EMSCRIPTEN_TARGET_ARGS_PREJS)
        message(FATAL_ERROR "You must provide a prejs script that loads your app.")
    endif()

    if (NOT NUI_ADD_EMSCRIPTEN_TARGET_ARGS_SOURCE_DIR)
        get_target_property(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_SOURCE_DIR} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} SOURCE_DIR)
    else()
        set(SOURCE_DIR ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_SOURCE_DIR})
    endif()

    string(REPLACE "-" "_" targetNormalized ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET})

    message(STATUS "emcmake: ${EMCMAKE}")

    ExternalProject_Add(
        "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten"
        SOURCE_DIR "${SOURCE_DIR}"
        CONFIGURE_COMMAND ${EMCMAKE} cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_CMAKE_OPTIONS} ${SOURCE_DIR}
        COMMAND $<TARGET_FILE:parcel-adapter> ${CMAKE_BINARY_DIR}/module_build/package.json "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}"
        BUILD_COMMAND ${EMMAKE} make ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_MAKE_OPTIONS} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel
        COMMAND $<TARGET_FILE:bin2hpp> ${CMAKE_BINARY_DIR}/module_build/bin/${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}.html ${CMAKE_BINARY_DIR}/include/${targetNormalized}.hpp ${targetNormalized}
        BINARY_DIR "${CMAKE_BINARY_DIR}/module_build"
        BUILD_ALWAYS 1
        INSTALL_COMMAND ""
    )
    add_dependencies(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten)
    add_dependencies(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten bin2hpp)
    add_custom_target(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-prejs ALL
        DEPENDS ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_PREJS}
    )
    add_dependencies(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-prejs
    )
endfunction()
