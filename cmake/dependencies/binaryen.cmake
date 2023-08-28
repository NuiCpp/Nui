option(NUI_FETCH_BINARYEN "Fetch binaryen" ON)

if (WIN32)
    set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_114/binaryen-version_114-x86_64-windows.tar.gz" CACHE STRING "Binaryen url")
elseif(UNIX)
    set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_114/binaryen-version_114-x86_64-linux.tar.gz" CACHE STRING "Binaryen url")
endif()

if (NUI_FETCH_BINARYEN)
    include(FetchContent)
    FetchContent_Declare(
        binaryen_release
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL "${NUI_BINARYEN_URL}"
    )
    FetchContent_GetProperties(binaryen_release)
    if (binaryen_release_POPULATED)
    else()
        FetchContent_Populate(binaryen_release)
    endif()
endif()