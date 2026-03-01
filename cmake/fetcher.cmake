include(FetchContent)

function(nui_fetch_dependency)
    cmake_parse_arguments(
        NUI_FETCH_ARGS
        ""
        "FIND;FETCH;LIBRARY_NAME;PACKAGE_NAME;FETCH_NAME;GIT_REPOSITORY;GIT_TAG;CONFIG"
        ""
        ${ARGN}
    )

    if (NUI_FIND_PACKAGE_CONFIG OR NUI_FETCH_ARGS_CONFIG)
        set(NUI_FETCH_ARGS_CONFIG ON)
    else()
        set(NUI_FETCH_ARGS_CONFIG OFF)
    endif()

    if (NOT NUI_FETCH_ARGS_PACKAGE_NAME)
        if (NOT NUI_FETCH_ARGS_LIBRARY_NAME)
            message(FATAL_ERROR "NUI_FETCH_ARGS_PACKAGE_NAME is not set")
        endif()
        set(NUI_FETCH_ARGS_PACKAGE_NAME ${NUI_FETCH_ARGS_LIBRARY_NAME})
    endif()
    if (NOT NUI_FETCH_ARGS_FETCH_NAME)
        if (NOT NUI_FETCH_ARGS_LIBRARY_NAME)
            message(FATAL_ERROR "NUI_FETCH_ARGS_FETCH_NAME is not set")
        endif()
        set(NUI_FETCH_ARGS_FETCH_NAME ${NUI_FETCH_ARGS_LIBRARY_NAME})
    endif()

    if (${NUI_FETCH_ARGS_FIND})
        message(STATUS "Trying to find ${NUI_FETCH_ARGS_PACKAGE_NAME}")
        if (${NUI_FETCH_ARGS_CONFIG})
            message(STATUS "Using CONFIG mode for find_package for ${NUI_FETCH_ARGS_PACKAGE_NAME}")
            find_package(${NUI_FETCH_ARGS_PACKAGE_NAME} CONFIG QUIET)
        else()
            message(STATUS "Using non-CONFIG find_package for ${NUI_FETCH_ARGS_PACKAGE_NAME}")
            find_package(${NUI_FETCH_ARGS_PACKAGE_NAME} QUIET)
        endif()

        if (${NUI_FETCH_ARGS_PACKAGE_NAME}_FOUND)
            message(STATUS "Found ${NUI_FETCH_ARGS_PACKAGE_NAME} via find_package")
            return()
        elseif(NOT ${NUI_FETCH_ARGS_FETCH})
            message(FATAL_ERROR "Could not find ${NUI_FETCH_ARGS_PACKAGE_NAME} and fetching is disabled")
        endif()
    endif()

    if (${NUI_FETCH_ARGS_FETCH})
        message(STATUS "Fetching ${NUI_FETCH_ARGS_FETCH_NAME}")
        FetchContent_Declare(
            ${NUI_FETCH_ARGS_FETCH_NAME}
            GIT_REPOSITORY ${NUI_FETCH_ARGS_GIT_REPOSITORY}
            GIT_TAG        ${NUI_FETCH_ARGS_GIT_TAG}
        )
        FetchContent_MakeAvailable(${NUI_FETCH_ARGS_FETCH_NAME})
    else()
        message(STATUS "${NUI_FETCH_ARGS_PACKAGE_NAME} was not found and fetching is disabled")
    endif()
endfunction()