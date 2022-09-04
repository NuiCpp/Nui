# TODO: Allow emscripten args to be passed
function(nui_prepare_emscripten_target)
    cmake_parse_arguments(
        NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS
        ""
        "TARGET;PREJS"
        "EMSCRIPTEN_ARGS"
        ${ARGN}
    )

    nui_set_target_output_directories(${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_TARGET})

    set_target_properties(
        ${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_TARGET}
        PROPERTIES
            LINK_FLAGS
            "-sSINGLE_FILE -sDEMANGLE_SUPPORT=1 -sNO_DISABLE_EXCEPTION_CATCHING -sNO_EXIT_RUNTIME=1 ${EMSCRIPTEN_ARGS} --no-entry --bind --pre-js=\"${NUI_PREPARE_EMSCRIPTEN_TARGET_ARGS_PREJS}\""
    )
endfunction()
