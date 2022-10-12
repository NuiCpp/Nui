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
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/.emscripten
        COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk install latest --build=Release
        COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk activate latest
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/emscripten-src
    )
else()
    # FIXME: fix config.
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/.emscripten
        COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk install latest --build=Release
        COMMAND ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcc --generate-config
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/emscripten-src
    )
endif()
add_custom_target(
    emscripten-setup
    DEPENDS ${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/.emscripten
)

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
        # emscripten cmake with passed down Release/Debug build type
        CONFIGURE_COMMAND 
            ${EMCMAKE} cmake 
                -DCMAKE_CXX_STANDARD=20 
                -DCMAKE_EXPORT_COMPILE_COMMANDS=1 
                "$<$<CONFIG:DEBUG>:-DCMAKE_BUILD_TYPE=Debug>"
                "$<$<CONFIG:RELEASE>:-DCMAKE_BUILD_TYPE=Release>"
                ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_CMAKE_OPTIONS} 
                ${SOURCE_DIR}
        # copy over package.json and fill parcel options that do not exist on it
        BUILD_COMMAND $<TARGET_FILE:parcel-adapter> ${SOURCE_DIR}/package.json ${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/package.json "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}"
        # emscripten make
        COMMAND ${EMMAKE} make ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_MAKE_OPTIONS} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel
        # rename to plain index.js
        COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}.js ${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.js
        # convert result to header file containing the page
        COMMAND $<TARGET_FILE:bin2hpp> ${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.html ${CMAKE_BINARY_DIR}/include/index.hpp index
        BINARY_DIR "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}"
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
    add_dependencies(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten
        emscripten-setup
    )
endfunction()
