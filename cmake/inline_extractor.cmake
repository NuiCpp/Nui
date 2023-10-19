function(nui_preprocess_inline_js)
    set(one_value_args TARGET SOURCE OUTPUT DIRECTORY INLINE_CACHE DEPENDS IS_FIRST)
    set(multi_value_args EXTRA_CXX_FLAGS)
    cmake_parse_arguments(CPP "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    string(TOUPPER ${CMAKE_BUILD_TYPE} build_type)
    string(REPLACE " " ";" c_flags "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${build_type}}")

    add_custom_command(
        OUTPUT ${CPP_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CPP_DIRECTORY}
        COMMAND ${CMAKE_CXX_COMPILER}
            #"-D$<JOIN:$<TARGET_PROPERTY:${CPP_TARGET},COMPILE_DEFINITIONS>,;-D>"
            "-I$<JOIN:$<TARGET_PROPERTY:${CPP_TARGET},INCLUDE_DIRECTORIES>,;-I>"
            ${c_flags}
            $<TARGET_PROPERTY:${CPP_TARGET},COMPILE_OPTIONS>
            ${CPP_EXTRA_CXX_FLAGS}
            -E ${CPP_SOURCE} -o ${CPP_OUTPUT}
        # COMMAND "${NUI_INLINE_EXTRACTOR_TARGET_FILE}" ${CPP_INLINE_CACHE} ${CPP_OUTPUT} ${CPP_IS_FIRST}
        COMMAND_EXPAND_LISTS VERBATIM
        IMPLICIT_DEPENDS C ${CPP_SOURCE}
        DEPENDS ${CPP_SOURCE} ${CPP_DEPENDS})
endfunction()

function(nui_enable_inline)
    set(one_value_args TARGET UNPACKED_MODE)
    set(multi_value_args)
    cmake_parse_arguments(nui_enable_inline_ARGS "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    get_target_property(INLINE_JS_SOURCES ${nui_enable_inline_ARGS_TARGET} SOURCES)

    set(INLINE_DIRECTORY_SUBDIR "nui-inline")
    set(INLINE_DIRECTORY "${CMAKE_BINARY_DIR}/${INLINE_DIRECTORY_SUBDIR}")
    set(INLINE_CACHE "${INLINE_DIRECTORY}/inline.cache")
    set(INLINE_IMPORTS_SCRIPTS "${INLINE_DIRECTORY}/inline_imports.js")
    set(INLINE_IMPORTS_STYLES "${INLINE_DIRECTORY}/inline_imports.css")

    # for each source file preprocess it:
    set(IS_FIRST TRUE)
    foreach(source_file ${INLINE_JS_SOURCES})
        get_filename_component(source_file_name "${source_file}" NAME)
        set(preprocessed_source_file "${INLINE_DIRECTORY}/${source_file_name}.i")
        nui_preprocess_inline_js(
            TARGET ${nui_enable_inline_ARGS_TARGET}
            DIRECTORY "${INLINE_DIRECTORY}"
            INLINE_CACHE "${INLINE_CACHE}"
            SOURCE "${source_file}"
            OUTPUT "${preprocessed_source_file}"
            EXTRA_CXX_FLAGS -P -CC -DNUI_INLINE -DNUI_MODULE_SOURCE_DIR="${CMAKE_SOURCE_DIR}" -DNUI_MODULE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
            IS_FIRST ${IS_FIRST}
        )
        set(IS_FIRST FALSE)
        message(STATUS "Preprocessing ${source_file} to ${preprocessed_source_file}")
        list(APPEND preprocessed_sources "${preprocessed_source_file}")
    endforeach()

    add_custom_command(
        OUTPUT
            ${INLINE_IMPORTS_SCRIPTS}
            ${INLINE_IMPORTS_STYLES}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${INLINE_DIRECTORY}
        COMMAND "${NUI_INLINE_EXTRACTOR_TARGET_FILE}" ${INLINE_CACHE} ${CMAKE_BINARY_DIR} ${INLINE_DIRECTORY_SUBDIR} ${preprocessed_sources}
        COMMAND_EXPAND_LISTS VERBATIM
        DEPENDS ${preprocessed_sources}
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

    if (NOT ${nui_enable_inline_ARGS_UNPACKED_MODE})
        add_custom_command(
            OUTPUT
                "${CMAKE_BINARY_DIR}/index_inserts.html"
            COMMAND ${NUI_INLINE_INJECTOR_TARGET_FILE} "${CMAKE_BINARY_DIR}/module_${nui_enable_inline_ARGS_TARGET}/bin/index.html" ${INLINE_IMPORTS_SCRIPTS} ${INLINE_IMPORTS_STYLES}
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