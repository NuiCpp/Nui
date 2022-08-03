function(nui_prepare_emscripten_target target)
    nui_set_target_output_directories(${target})

  # insert --pre-js=\"../cmake/emsdk_prejs.js in the future
    set_target_properties(
        ${target}
        PROPERTIES
            LINK_FLAGS
            "-sSINGLE_FILE -s DEMANGLE_SUPPORT=1 --no-entry --bind -s NO_EXIT_RUNTIME=1"
    )
endfunction()
