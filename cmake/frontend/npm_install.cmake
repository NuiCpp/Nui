function(add_npm_install)
    cmake_parse_arguments(
        NPM
        ""                                              # no boolean options
        "WORKING_DIRECTORY;STAMP_FILE;NPM_EXECUTABLE;TARGET_NAME"   # single-value args
        "OUTPUTS;DEPENDS;INSTALL_ARGS"                   # multi-value args
        ${ARGN}
    )
    message(STATUS "Configuring npm install as target ${NPM_TARGET_NAME}")

    if(NOT NPM_WORKING_DIRECTORY)
        message(FATAL_ERROR "add_npm_install: WORKING_DIRECTORY is required")
    endif()

    # Default npm executable
    if(NOT NPM_NPM_EXECUTABLE)
        set(NPM_NPM_EXECUTABLE "npm")
    endif()

    # Default stamp file
    if(NOT NPM_STAMP_FILE)
        set(NPM_STAMP_FILE
            ${CMAKE_BINARY_DIR}/${NPM_TARGET_NAME}_npm_install.stamp
        )
    endif()

    # Default dependencies
    if(NOT NPM_DEPENDS)
        set(NPM_DEPENDS
            ${NPM_WORKING_DIRECTORY}/package.json
        )
    endif()

    message(STATUS "NPM outputs: ${NPM_OUTPUTS}")

    add_custom_command(
        OUTPUT
            ${NPM_STAMP_FILE}
            ${NPM_OUTPUTS}

        COMMAND
            ${NPM_NPM_EXECUTABLE}
            install
            ${NPM_INSTALL_ARGS}

        COMMAND
            ${CMAKE_COMMAND} -E touch ${NPM_STAMP_FILE}

        WORKING_DIRECTORY
            ${NPM_WORKING_DIRECTORY}

        DEPENDS
            ${NPM_DEPENDS}

        COMMENT
            "Running npm install in ${NPM_WORKING_DIRECTORY}"

        VERBATIM
    )

    add_custom_target(
        ${NPM_TARGET_NAME}
        DEPENDS
            ${NPM_STAMP_FILE}
            ${NPM_OUTPUTS}
    )
endfunction()