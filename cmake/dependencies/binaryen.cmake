option(NUI_FETCH_BINARYEN "Fetch binaryen" ON)

if (WIN32)
    set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_124/binaryen-version_124-x86_64-windows.tar.gz" CACHE STRING "Binaryen url")
elseif(APPLE)
    if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_124/binaryen-version_124-arm64-macos.tar.gz" CACHE STRING "Binaryen url")
    else()
        set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_124/binaryen-version_124-x86_64-macos.tar.gz" CACHE STRING "Binaryen url")
    endif()
elseif(UNIX)
    set(NUI_BINARYEN_URL "https://github.com/WebAssembly/binaryen/releases/download/version_124/binaryen-version_124-x86_64-linux.tar.gz" CACHE STRING "Binaryen url")
endif()

if (NUI_FETCH_BINARYEN)
    include(FetchContent)
    FetchContent_Declare(
        binaryen_release
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL "${NUI_BINARYEN_URL}"
    )
    FetchContent_MakeAvailable(binaryen_release)
endif()