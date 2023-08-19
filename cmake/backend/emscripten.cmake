project(emscripten-git NONE)

include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
    emscripten
    GIT_REPOSITORY https://github.com/emscripten-core/emsdk.git
    GIT_TAG        17f6a2ef92f198f3c9ff30d07664e4090a0ecaf7
)

FetchContent_MakeAvailable(emscripten)

if(UNIX)
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/.emscripten"
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" install latest --build=Release
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" activate latest
        COMMAND $<TARGET_FILE:patch-acorn> "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/tools/acorn-optimizer.js"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/emscripten-src"
    )
else()
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/.emscripten"
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" install latest --build=Release
        COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/emcc" --generate-config
        COMMAND $<TARGET_FILE:patch-acorn> "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/tools/acorn-optimizer.js"
        COMMAND $<TARGET_FILE:patch-emscripten-config> 
            "${CMAKE_BINARY_DIR}/_deps/emscripten-src/upstream/emscripten/.emscripten" 
            "${CMAKE_BINARY_DIR}/_deps/binaryen_release-src" 
            "${CMAKE_BINARY_DIR}/_deps/emscripten-src/java/bin/java.exe"
            # not setting node, because global installed node might be preferred
            # "${CMAKE_BINARY_DIR}/_deps/emscripten-src/node/bin/node.exe"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/emscripten-src"
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
        "CMAKE_OPTIONS"
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

    get_target_property(TARGET_TYPE ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} TYPE)

    string(REPLACE "-" "_" targetNormalized ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET})

    message(STATUS "emcmake: ${EMCMAKE}")
    
    if (${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
        set(ENABLE_BIN2HPP "no")
    else()
        set(ENABLE_BIN2HPP "yes")
    endif()

    ExternalProject_Add(
        "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten"
        SOURCE_DIR "${SOURCE_DIR}"
        # emscripten cmake with passed down Release/Debug build type
        CONFIGURE_COMMAND 
            ${EMCMAKE} cmake 
                -DCMAKE_CXX_STANDARD=23 
                -DCMAKE_EXPORT_COMPILE_COMMANDS=1 
                ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_CMAKE_OPTIONS} 
                "${SOURCE_DIR}"
        # copy over package.json and fill parcel options that do not exist on it
        BUILD_COMMAND $<TARGET_FILE:parcel-adapter> "${SOURCE_DIR}/package.json" "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/package.json" "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}"
        # emscripten make
        COMMAND cmake --build "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}" --target ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel
        # convert result to header file containing the page
        COMMAND $<TARGET_FILE:bin2hpp> ${ENABLE_BIN2HPP} "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.html" "${CMAKE_BINARY_DIR}/include/index.hpp" index
        BINARY_DIR "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}"
        BUILD_ALWAYS 1
        BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.html"
        INSTALL_COMMAND ""
    )
    add_dependencies(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten)
    add_dependencies(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten bin2hpp)
    add_custom_target(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-prejs ALL
        DEPENDS ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_PREJS}
    )
    add_custom_target(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel-dep 
        DEPENDS ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten
    )
    add_dependencies(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-prejs
        emscripten-setup
    )
    add_dependencies(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel-dep
        emscripten-setup
    )

    if (${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
        target_include_directories(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} INTERFACE "${CMAKE_BINARY_DIR}/include")
    else()
        target_include_directories(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} PRIVATE "${CMAKE_BINARY_DIR}/include")
    endif()
endfunction()
