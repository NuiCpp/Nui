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
            # Not patching node
            # "${NUI_NODE}"
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
        "DISABLE_BIN2HPP;DISABLE_PARCEL_ADAPTER"
        "TARGET;PREJS;SOURCE_DIR"
        "CMAKE_OPTIONS"
        ${ARGN}
    )

    if (NOT NUI_ADD_EMSCRIPTEN_TARGET_ARGS_DISABLE_BIN2HPP)
        set(ENABLE_BIN2HPP ON)
    else()
        if (NUI_ADD_EMSCRIPTEN_TARGET_ARGS_DISABLE_BIN2HPP)
            set(ENABLE_BIN2HPP OFF)
        else()
            if (${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
                set(ENABLE_BIN2HPP OFF)
            else()
                set(ENABLE_BIN2HPP ON)
            endif()
        endif()
    endif()

    if (NOT NUI_ADD_EMSCRIPTEN_TARGET_ARGS_DISABLE_PARCEL_ADAPTER)
        set(ENABLE_PARCEL_ADAPTER ON)
    else()
        if (NUI_ADD_EMSCRIPTEN_TARGET_ARGS_DISABLE_PARCEL_ADAPTER)
            set(ENABLE_PARCEL_ADAPTER OFF)
        else()
            set(ENABLE_PARCEL_ADAPTER ON)
        endif()
    endif()

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
    endif()

    if (ENABLE_PARCEL_ADAPTER)
        set(BUILD_COMMAND BUILD_COMMAND $<TARGET_FILE:parcel-adapter> "${SOURCE_DIR}/package.json" "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/package.json" "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}")
    else()
        set(BUILD_COMMAND BUILD_COMMAND cmake -E copy "${SOURCE_DIR}/package.json" "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/package.json")
    endif()

    if (ENABLE_BIN2HPP AND ${ENABLE_BIN2HPP})
        set(BIN2HPP_COMMAND COMMAND $<TARGET_FILE:bin2hpp> "on" "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.html" "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/include/index.hpp" index)
    else()
        set(BIN2HPP_COMMAND COMMAND cmake -E true)
    endif()

    include(ExternalProject)
    ExternalProject_Add(
        "${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten"
        SOURCE_DIR "${SOURCE_DIR}"
        # emscripten cmake with passed down Release/Debug build type
        CONFIGURE_COMMAND
            ${EMCMAKE} cmake
                ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_CMAKE_OPTIONS}
                "-DNUI_NPM=${NUI_NPM}"
                "-DNUI_NODE=${NUI_NODE}"
                -DNUI_INLINE_EXTRACTOR_TARGET_FILE=$<TARGET_FILE:inline-parser>
                -DNUI_INLINE_INJECTOR_TARGET_FILE=$<TARGET_FILE:inline-injector>
                -DNUI_MODULE_BUILD_DIR=${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}
                "${SOURCE_DIR}"
        # copy over package.json and fill parcel options that do not exist on it
        ${BUILD_COMMAND}
        # emscripten make
        COMMAND cmake --build "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}" --target ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel
        # convert result to header file containing the page
        ${BIN2HPP_COMMAND}
        BINARY_DIR "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}"
        BUILD_ALWAYS 1
        BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.html"
        INSTALL_COMMAND ""
        DEPENDS inline-parser inline-injector parcel-adapter
    )
    add_dependencies(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET} ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten)
    add_dependencies(${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten bin2hpp)
    add_custom_target(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-prejs ALL
        DEPENDS ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_PREJS}
    )
    add_custom_target(
        ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-parcel-dep
        DEPENDS
            ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}-emscripten
            "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/bin/index.html"
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
        target_include_directories(
            ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}
            INTERFACE
                "${CMAKE_BINARY_DIR}/include"
                "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/include"
        )
    else()
        target_include_directories(
            ${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}
            PRIVATE
                "${CMAKE_BINARY_DIR}/include"
                "${CMAKE_BINARY_DIR}/module_${NUI_ADD_EMSCRIPTEN_TARGET_ARGS_TARGET}/include"
        )
    endif()
endfunction()
