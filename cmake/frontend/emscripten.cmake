# TODO: Allow emscripten args to be passed
function(nui_prepare_emscripten_target)
    cmake_parse_arguments(
        NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS
        ""
        "TARGET;PREJS"
        "EMSCRIPTEN_LINK_OPTIONS;EMSCRIPTEN_COMPILE_OPTIONS"
        ${ARGN}
    )

    nui_set_target_output_directories(${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_TARGET})

    string (REPLACE ";" " " EMSCRIPTEN_LINK "${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_EMSCRIPTEN_LINK_OPTIONS}")
    target_compile_options(${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_TARGET} PRIVATE ${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_EMSCRIPTEN_COMPILE_OPTIONS})
    set_target_properties(
        ${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_TARGET}
        PROPERTIES
            LINK_FLAGS
            "-sSINGLE_FILE -sNO_EXIT_RUNTIME=1 ${EMSCRIPTEN_LINK} --no-entry --bind --pre-js=\"${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_PREJS}\""
    )
endfunction()
