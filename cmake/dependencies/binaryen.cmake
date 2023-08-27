option(NUI_FETCH_BINARYEN "Fetch binaryen" ON)
set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_112/binaryen-version_112-x86_64-windows.tar.gz" CACHE STRING "Binaryen url")

if (NUI_FETCH_BINARYEN)
    include(FetchContent)
    FetchContent_Declare(
        binaryen
        URL ${NUI_BINARYEN_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_GetProperties(binaryen)
    if (binaryen_POPULATED)
    else()
        FetchContent_Populate(binaryen)
    endif()
endif()