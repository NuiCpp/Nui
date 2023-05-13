function(nui_set_project_warnings target_name)
    if (${MSVC})
        set(CLANG_WARNINGS -Wmost)
        list(APPEND CLANG_WARNINGS -Wextra)
        set(GCC_WARNINGS -Wall)
    else()
        set(CLANG_WARNINGS -Wall)
        list(APPEND CLANG_WARNINGS -Wextra)
        set(GCC_WARNINGS -Wall)
    endif()

    list(APPEND CLANG_WARNINGS
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual
                        # destructor. This helps catch hard to track down memory errors
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
    )

    list(  
        APPEND GCC_WARNINGS
        # -Wshadow is too aggressive on gcc and does not understand lambdas well
        -Wold-style-cast
        -Wunused
        -Wnon-virtual-dtor
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion)

    if(WARNINGS_AS_ERRORS)
        list(CLANG_WARNINGS APPEND -Werror)
        list(GCC_WARNINGS APPEND -Werror)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*GNU")
        set(PROJECT_WARNINGS ${GCC_WARNINGS})
    else()
        message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
    endif()

    target_compile_options(${target_name} PRIVATE ${PROJECT_WARNINGS})

endfunction()
