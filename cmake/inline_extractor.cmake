function(nui_preprocess_inline_js)
    set(one_value_args TARGET SOURCE OUTPUT DIRECTORY INLINE_CACHE DEPENDS IS_FIRST)
    set(multi_value_args EXTRA_CXX_FLAGS)
    cmake_parse_arguments(CPP "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(CURRENT_BUILD_TYPE "Release")
    if (CMAKE_BUILD_TYPE)
        set(CURRENT_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    endif()

    string(TOUPPER ${CURRENT_BUILD_TYPE} PREPROCESS_BUILD_TYPE)
    string(REPLACE " " ";" PREPROCESS_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${PREPROCESS_BUILD_TYPE}}")

    set(INCLUDE_DIRS "$<TARGET_PROPERTY:${CPP_TARGET},INCLUDE_DIRECTORIES>")
    set(COMPILE_DEFINITIONS "$<TARGET_PROPERTY:${CPP_TARGET},COMPILE_DEFINITIONS>")
    add_custom_command(
        OUTPUT ${CPP_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CPP_DIRECTORY}
        COMMAND ${CMAKE_CXX_COMPILER}
            "$<$<BOOL:${COMPILE_DEFINITIONS}>:-D$<JOIN:${COMPILE_DEFINITIONS},;-D>>"
            "$<$<BOOL:${INCLUDE_DIRS}>:-I$<JOIN:${INCLUDE_DIRS},;-I>>"
            ${PREPROCESS_CXX_FLAGS}
            $<TARGET_PROPERTY:${CPP_TARGET},COMPILE_OPTIONS>
            ${CPP_EXTRA_CXX_FLAGS}
            -E ${CPP_SOURCE} -o ${CPP_OUTPUT}
        COMMAND_EXPAND_LISTS VERBATIM
        IMPLICIT_DEPENDS C ${CPP_SOURCE}
        DEPENDS ${CPP_SOURCE} ${CPP_DEPENDS})
endfunction()

function(nui_enable_inline)
    set(one_value_args TARGET UNPACKED_MODE RELATIVE_TO CXX_STANDARD)
    set(multi_value_args)
    cmake_parse_arguments(nui_enable_inline_ARGS "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    get_target_property(INLINE_SOURCES ${nui_enable_inline_ARGS_TARGET} SOURCES)
    message(STATUS "Inline sources for ${nui_enable_inline_ARGS_TARGET}")

    set(INLINE_DIRECTORY_SUBDIR "nui-inline")
    set(INLINE_DIRECTORY "${NUI_MODULE_BUILD_DIR}/${INLINE_DIRECTORY_SUBDIR}")
    set(INLINE_CACHE "${INLINE_DIRECTORY}/inline.cache")
    set(INLINE_IMPORTS_SCRIPTS "${INLINE_DIRECTORY}/inline_imports.js")
    set(INLINE_IMPORTS_STYLES "${INLINE_DIRECTORY}/inline_imports.css")

    if (NOT nui_enable_inline_ARGS_CXX_STANDARD)
        set(nui_enable_inline_ARGS_CXX_STANDARD 20)
    endif()

    # for each source file preprocess it:
    set(IS_FIRST TRUE)
    foreach(SOURCE_FILE ${INLINE_SOURCES})
        # Get relative path to source file
        file(RELATIVE_PATH SOURCE_FILE_NAME_RELATIVE "${nui_enable_inline_ARGS_RELATIVE_TO}" "${SOURCE_FILE}")
        get_filename_component(SOURCE_FILE_NAME "${SOURCE_FILE_NAME_RELATIVE}" NAME_WE)
        # Get just the directory part of the source file
        get_filename_component(SOURCE_FILE_DIR "${SOURCE_FILE_NAME_RELATIVE}" DIRECTORY)
        if (NOT "${SOURCE_FILE_DIR}" STREQUAL "")
            set(PREPROCESSED_SOURCE_FILE "${INLINE_DIRECTORY}/${SOURCE_FILE_DIR}/${SOURCE_FILE_NAME}.i")
        else()
            set(PREPROCESSED_SOURCE_FILE "${INLINE_DIRECTORY}/${SOURCE_FILE_NAME}.i")
        endif()
        nui_preprocess_inline_js(
            TARGET ${nui_enable_inline_ARGS_TARGET}
            DIRECTORY "${INLINE_DIRECTORY}/${SOURCE_FILE_DIR}"
            INLINE_CACHE "${INLINE_CACHE}"
            SOURCE "${SOURCE_FILE}"
            OUTPUT "${PREPROCESSED_SOURCE_FILE}"
            CXX_STANDARD ${nui_enable_inline_ARGS_CXX_STANDARD}
            EXTRA_CXX_FLAGS
                -P -CC -DNUI_INLINE
                -DNUI_MODULE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
                -DNUI_MODULE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                -std=c++${nui_enable_inline_ARGS_CXX_STANDARD}
            IS_FIRST ${IS_FIRST}
        )
        set(IS_FIRST FALSE)
        list(APPEND PREPROCESSED_SOURCES "${PREPROCESSED_SOURCE_FILE}")
    endforeach()

    add_custom_command(
        OUTPUT
            ${INLINE_IMPORTS_SCRIPTS}
            ${INLINE_IMPORTS_STYLES}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INLINE_DIRECTORY}
        COMMAND "${NUI_INLINE_EXTRACTOR_TARGET_FILE}" ${INLINE_CACHE} ${CMAKE_BINARY_DIR} ${INLINE_DIRECTORY_SUBDIR} ${PREPROCESSED_SOURCES}
        COMMAND_EXPAND_LISTS VERBATIM
        DEPENDS ${PREPROCESSED_SOURCES}
    )

    add_custom_target(
        nui-inline-${nui_enable_inline_ARGS_TARGET}
        ALL
            DEPENDS ${INLINE_IMPORTS_SCRIPTS}
            DEPENDS ${INLINE_IMPORTS_STYLES}
    )
    add_dependencies(${nui_enable_inline_ARGS_TARGET} nui-inline-${nui_enable_inline_ARGS_TARGET})

    if (NOT nui_enable_inline_ARGS_UNPACKED_MODE)
        set(nui_enable_inline_ARGS_UNPACKED_MODE off)
    endif()

    set(NUI_DEFER_INLINE_SCRIPTS_TAG "nodefer")
    if (NUI_DEFER_INLINE_SCRIPTS)
        set(NUI_DEFER_INLINE_SCRIPTS_TAG "defer")
    endif()

    if (NOT ${nui_enable_inline_ARGS_UNPACKED_MODE})
        add_custom_command(
            OUTPUT
                "${CMAKE_BINARY_DIR}/index_inserts.html"
            COMMAND ${NUI_INLINE_INJECTOR_TARGET_FILE} "${CMAKE_BINARY_DIR}/module_${nui_enable_inline_ARGS_TARGET}/bin/index.html" ${INLINE_IMPORTS_SCRIPTS} ${INLINE_IMPORTS_STYLES} ${NUI_DEFER_INLINE_SCRIPTS_TAG}
            DEPENDS
                ${INLINE_IMPORTS_SCRIPTS}
                ${INLINE_IMPORTS_STYLES}
                "${CMAKE_BINARY_DIR}/module_${nui_enable_inline_ARGS_TARGET}/bin/index.html"
        )

        add_custom_target(
            nui-inline-inject-${nui_enable_inline_ARGS_TARGET}
            ALL
                DEPENDS "${CMAKE_BINARY_DIR}/index_inserts.html"
        )
    endif()
endfunction()