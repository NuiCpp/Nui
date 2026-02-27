include(FetchContent)

function(nui_fetch_dependency)
    cmake_parse_arguments(
        NUI_FETCH_ARGS
        ""
        "FIND;FETCH;LIBRARY_NAME;PACKAGE_NAME;FETCH_NAME;GIT_REPOSITORY;GIT_TAG"
        ""
        ${ARGN}
    )

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

    if (NUI_FETCH_ARGS_FIND)
        find_package(${NUI_FETCH_ARGS_PACKAGE_NAME})

        if (${NUI_FETCH_ARGS_PACKAGE_NAME}_FOUND)
            return()
        endif()
    endif()

    if (NUI_FETCH_ARGS_FETCH AND ${NUI_FETCH_ARGS_FETCH})
        message(STATUS "Fetching ${NUI_FETCH_ARGS_FETCH_NAME}")
        FetchContent_Declare(
            ${NUI_FETCH_ARGS_FETCH_NAME}
            GIT_REPOSITORY ${NUI_FETCH_ARGS_GIT_REPOSITORY}
            GIT_TAG        ${NUI_FETCH_ARGS_GIT_TAG}
        )
        FetchContent_MakeAvailable(${NUI_FETCH_ARGS_FETCH_NAME})
    endif()
endfunction()