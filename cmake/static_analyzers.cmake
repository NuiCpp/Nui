if(NUI_ENABLE_CLANG_TIDY)
  find_program(CLANGTIDY clang-tidy)
  if(CLANGTIDY)
    message(STATUS "clang-tidy found: ${CLANGTIDY}")
    set(CMAKE_CXX_CLANG_TIDY
        ${CLANGTIDY} -extra-arg=-Wno-unknown-warning-option --warnings-as-errors=* #-header-filter=.
        --line-filter=[{\"name\":\"win_thread.ipp\",\"lines\":[[79,79]]}])
  else()
    message(SEND_ERROR "clang-tidy requested but executable not found")
  endif()
endif()