# TODO: Allow emscripten args to be passed
function(nui_prepare_emscripten_target target prejs)
    nui_set_target_output_directories(${target})

    set_target_properties(
        ${target}
        PROPERTIES
            LINK_FLAGS
            "-sSINGLE_FILE -sDEMANGLE_SUPPORT=1 -sNO_DISABLE_EXCEPTION_CATCHING --no-entry --bind -s NO_EXIT_RUNTIME=1 --pre-js=\"${prejs}\""
    )
endfunction()
