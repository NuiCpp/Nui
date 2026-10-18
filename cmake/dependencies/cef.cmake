# CEF browser engine (NUI_BROWSER_ENGINE=cef).
#
# Sources of the distribution, first match wins:
#   NUI_CEF_SYSTEM=ON   the system package (Arch: /usr/lib/cef, /usr/include/cef, /usr/src/cef)
#   NUI_CEF_ROOT        an extracted binary distribution
#   NUI_CEF_ARCHIVE     a local distribution archive, checked against the pinned SHA256 if it is the pinned one
#   NUI_FETCH_CEF=ON    downloads the pinned distribution into NUI_CEF_DOWNLOAD_DIR
# Offline builds turn NUI_FETCH_CEF off and pass one of the others, see cef_fetch.cmake.

include("${CMAKE_CURRENT_LIST_DIR}/cef_version.cmake")

option(NUI_CEF_SYSTEM "Use the CEF of the system (distribution package)" OFF)
set(NUI_CEF_ROOT "" CACHE PATH "Extracted CEF binary distribution")
set(NUI_CEF_ARCHIVE "" CACHE FILEPATH "Local CEF binary distribution archive")
option(NUI_FETCH_CEF "Download the pinned CEF distribution if no other source is given" ON)
set(NUI_CEF_PLATFORM "" CACHE STRING "CEF platform (linux64, linuxarm64, macosarm64, macosx64), empty for the target")
nui_cef_default_download_directory(nuiCefDefaultDownloadDirectory)
set(NUI_CEF_DOWNLOAD_DIR "${nuiCefDefaultDownloadDirectory}" CACHE PATH "Download and extraction directory for CEF")
set(NUI_CEF_API_VERSION "" CACHE STRING "CEF_API_VERSION, empty for CEF_API_VERSION_LAST of the CEF headers in use")
set(NUI_CEF_SYSTEM_LIBRARY_DIR "" CACHE PATH "Directory of the system libcef.so, empty to search")
set(NUI_CEF_SYSTEM_INCLUDE_DIR "" CACHE PATH "Directory containing the include/ directory of the system CEF headers")
set(NUI_CEF_SYSTEM_WRAPPER_DIR "" CACHE PATH "Directory containing libcef_dll/ (wrapper sources) of the system CEF")

if (APPLE)
    enable_language(OBJCXX)
endif()

if (NOT NUI_CEF_PLATFORM)
    set(nuiCefProcessor "${CMAKE_SYSTEM_PROCESSOR}")
    if (APPLE AND CMAKE_OSX_ARCHITECTURES)
        list(LENGTH CMAKE_OSX_ARCHITECTURES nuiCefArchitectureCount)
        if (NOT nuiCefArchitectureCount EQUAL 1)
            message(FATAL_ERROR "The CEF engine needs a single CMAKE_OSX_ARCHITECTURES value")
        endif()
        set(nuiCefProcessor "${CMAKE_OSX_ARCHITECTURES}")
    endif()
    nui_cef_platform_for("${CMAKE_SYSTEM_NAME}" "${nuiCefProcessor}" nuiCefPlatform)
else()
    set(nuiCefPlatform "${NUI_CEF_PLATFORM}")
endif()

# ---------------------------------------------------------------------------------------------------------------------
# Locate the distribution.
if (NUI_CEF_SYSTEM)
    if (APPLE)
        message(FATAL_ERROR "NUI_CEF_SYSTEM is supported on Linux only")
    endif()
    find_path(NUI_CEF_SYSTEM_LIBRARY_DIR_FOUND libcef.so
        HINTS "${NUI_CEF_SYSTEM_LIBRARY_DIR}"
        PATHS /usr/lib/cef /usr/lib64/cef /usr/local/lib/cef
        NO_DEFAULT_PATH
    )
    find_path(NUI_CEF_SYSTEM_INCLUDE_DIR_FOUND include/cef_version.h
        HINTS "${NUI_CEF_SYSTEM_INCLUDE_DIR}"
        PATHS /usr/include/cef /usr/local/include/cef
        NO_DEFAULT_PATH
    )
    file(GLOB nuiCefSystemWrapperCandidates LIST_DIRECTORIES true /usr/src/cef /usr/src/cef-*)
    find_path(NUI_CEF_SYSTEM_WRAPPER_DIR_FOUND libcef_dll/CMakeLists.txt
        HINTS "${NUI_CEF_SYSTEM_WRAPPER_DIR}"
        PATHS ${nuiCefSystemWrapperCandidates}
        NO_DEFAULT_PATH
    )
    foreach (part LIBRARY INCLUDE WRAPPER)
        if (NOT NUI_CEF_SYSTEM_${part}_DIR_FOUND)
            message(FATAL_ERROR "System CEF not found (${part}), set NUI_CEF_SYSTEM_${part}_DIR")
        endif()
    endforeach()
    set(nuiCefLibraryDirectory "${NUI_CEF_SYSTEM_LIBRARY_DIR_FOUND}")
    set(nuiCefResourceDirectory "${NUI_CEF_SYSTEM_LIBRARY_DIR_FOUND}")
    set(nuiCefIncludeRoot "${NUI_CEF_SYSTEM_INCLUDE_DIR_FOUND}")

    # Distribution packages ship the wrapper sources without the include/ directory next to them, which the wrapper's
    # source list expects. A build-tree copy with a link to the system headers restores the distribution layout.
    set(nuiCefWrapperRoot "${CMAKE_BINARY_DIR}/nui-cef-wrapper-source")
    file(REMOVE_RECURSE "${nuiCefWrapperRoot}")
    file(COPY "${NUI_CEF_SYSTEM_WRAPPER_DIR_FOUND}/libcef_dll" DESTINATION "${nuiCefWrapperRoot}")
    file(CREATE_LINK "${nuiCefIncludeRoot}/include" "${nuiCefWrapperRoot}/include" SYMBOLIC)
else()
    if (NUI_CEF_ROOT)
        set(nuiCefRoot "${NUI_CEF_ROOT}")
    elseif (NUI_CEF_ARCHIVE)
        get_filename_component(nuiCefArchiveName "${NUI_CEF_ARCHIVE}" NAME)
        nui_cef_archive_name(${nuiCefPlatform} nuiCefPinnedArchiveName)
        if (nuiCefArchiveName STREQUAL nuiCefPinnedArchiveName)
            file(SHA256 "${NUI_CEF_ARCHIVE}" nuiCefArchiveSha256)
            if (NOT nuiCefArchiveSha256 STREQUAL NUI_CEF_SHA256_${nuiCefPlatform})
                message(FATAL_ERROR "${NUI_CEF_ARCHIVE} does not match the pinned SHA256")
            endif()
        endif()
        nui_cef_extract("${NUI_CEF_ARCHIVE}" "${CMAKE_BINARY_DIR}/_deps/cef" nuiCefRoot)
    elseif (NUI_FETCH_CEF)
        nui_cef_download(${nuiCefPlatform} "${NUI_CEF_DOWNLOAD_DIR}" nuiCefDownloadedArchive)
        nui_cef_extract("${nuiCefDownloadedArchive}" "${NUI_CEF_DOWNLOAD_DIR}" nuiCefRoot)
    else()
        message(FATAL_ERROR "No CEF source: set NUI_CEF_ROOT, NUI_CEF_ARCHIVE or NUI_CEF_SYSTEM, or enable NUI_FETCH_CEF")
    endif()
    if (NOT EXISTS "${nuiCefRoot}/include/cef_version.h")
        message(FATAL_ERROR "${nuiCefRoot} is not a CEF binary distribution")
    endif()
    set(nuiCefLibraryDirectory "${nuiCefRoot}/Release")
    set(nuiCefResourceDirectory "${nuiCefRoot}/Resources")
    set(nuiCefIncludeRoot "${nuiCefRoot}")
    set(nuiCefWrapperRoot "${nuiCefRoot}")
endif()

file(STRINGS "${nuiCefIncludeRoot}/include/cef_version.h" nuiCefVersionLine REGEX "^#define CEF_VERSION \"")
string(REGEX REPLACE "^#define CEF_VERSION \"([^\"]*)\".*" "\\1" nuiCefVersion "${nuiCefVersionLine}")
if (NOT NUI_CEF_SYSTEM AND NOT nuiCefVersion STREQUAL NUI_CEF_PINNED_VERSION)
    message(WARNING "CEF ${nuiCefVersion} differs from the version Nui pins (${NUI_CEF_PINNED_VERSION})")
endif()

file(STRINGS "${nuiCefIncludeRoot}/include/cef_api_versions.h" nuiCefApiLines REGEX "^#define CEF_API_VERSION_(MIN|LAST) ")
foreach (line ${nuiCefApiLines})
    if (line MATCHES "CEF_API_VERSION_(MIN|LAST) CEF_API_VERSION_([0-9]+)")
        set(nuiCefApiVersion${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
    endif()
endforeach()
if (NUI_CEF_API_VERSION)
    set(nuiCefApiVersion "${NUI_CEF_API_VERSION}")
else()
    set(nuiCefApiVersion "${nuiCefApiVersionLAST}")
endif()
if (nuiCefApiVersion LESS NUI_CEF_API_VERSION_MIN)
    message(FATAL_ERROR "CEF API version ${nuiCefApiVersion} is older than Nui needs (${NUI_CEF_API_VERSION_MIN})")
endif()
if (nuiCefApiVersion LESS nuiCefApiVersionMIN OR nuiCefApiVersion GREATER nuiCefApiVersionLAST)
    message(FATAL_ERROR
        "CEF API version ${nuiCefApiVersion} outside of ${nuiCefApiVersionMIN}..${nuiCefApiVersionLAST} of CEF ${nuiCefVersion}")
endif()
message(STATUS "Nui: CEF ${nuiCefVersion}, API version ${nuiCefApiVersion}")

set(NUI_CEF_VERSION "${nuiCefVersion}" CACHE INTERNAL "")
set(NUI_CEF_LIBRARY_DIR "${nuiCefLibraryDirectory}" CACHE INTERNAL "")
set(NUI_CEF_RESOURCE_DIR "${nuiCefResourceDirectory}" CACHE INTERNAL "")
set(NUI_CEF_IS_SYSTEM "${NUI_CEF_SYSTEM}" CACHE INTERNAL "")

# ---------------------------------------------------------------------------------------------------------------------
# Headers, definitions and the wrapper (libcef_dll_wrapper, built from the distribution's sources).
add_library(nui-cef-headers INTERFACE)
target_include_directories(nui-cef-headers SYSTEM INTERFACE "${nuiCefIncludeRoot}")
target_compile_definitions(nui-cef-headers
    INTERFACE
        __STDC_CONSTANT_MACROS
        __STDC_FORMAT_MACROS
        CEF_API_VERSION=${nuiCefApiVersion}
        $<$<PLATFORM_ID:Linux>:_FILE_OFFSET_BITS=64>
)
target_compile_features(nui-cef-headers INTERFACE cxx_std_20)

# The only CEF macro the wrapper's CMakeLists uses. CEF's cef_variables.cmake is not included because it sets global
# state (CMAKE_BUILD_TYPE, CMAKE_OSX_*, -Werror) that belongs to the application.
macro(SET_LIBRARY_TARGET_PROPERTIES target)
    target_link_libraries(${target} PUBLIC nui-cef-headers)
    target_include_directories(${target} PRIVATE "${nuiCefWrapperRoot}")
    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_options(${target} PRIVATE -w)
endmacro()

block()
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory("${nuiCefWrapperRoot}/libcef_dll" "${CMAKE_BINARY_DIR}/nui-cef-wrapper" EXCLUDE_FROM_ALL)
endblock()

# ---------------------------------------------------------------------------------------------------------------------
# Runtime directory of the build tree (Linux). Links to every runtime file of the distribution; the helper is built
# into it. libcef is imported from there, so every executable linking nui-backend gets it as build RPATH, and
# libcef finds its resources next to itself.
set(NUI_CEF_RUNTIME_DIR "${CMAKE_BINARY_DIR}/nui-cef" CACHE INTERNAL "")
if (NOT APPLE)
    file(MAKE_DIRECTORY "${NUI_CEF_RUNTIME_DIR}")
    file(GLOB nuiCefOldLinks LIST_DIRECTORIES true "${NUI_CEF_RUNTIME_DIR}/*")
    foreach (entry ${nuiCefOldLinks})
        if (IS_SYMLINK "${entry}")
            file(REMOVE "${entry}")
        endif()
    endforeach()

    set(nuiCefRuntimeFiles "")
    file(GLOB nuiCefRuntimeCandidates LIST_DIRECTORIES true "${nuiCefLibraryDirectory}/*" "${nuiCefResourceDirectory}/*")
    list(REMOVE_DUPLICATES nuiCefRuntimeCandidates)
    foreach (entry ${nuiCefRuntimeCandidates})
        get_filename_component(name "${entry}" NAME)
        if (name MATCHES "^(libcef_dll_wrapper.*|.*\\.a|cmake|include)$")
            continue()
        endif()
        if (name STREQUAL "chrome-sandbox")
            # Only a root owned setuid helper is usable, the distribution's copy is neither.
            if (NOT NUI_CEF_SYSTEM)
                continue()
            endif()
        endif()
        file(CREATE_LINK "${entry}" "${NUI_CEF_RUNTIME_DIR}/${name}" SYMBOLIC)
        list(APPEND nuiCefRuntimeFiles "${entry}")
    endforeach()
    set(NUI_CEF_RUNTIME_FILES "${nuiCefRuntimeFiles}" CACHE INTERNAL "")

    add_library(nui-libcef SHARED IMPORTED GLOBAL)
    set_target_properties(nui-libcef
        PROPERTIES
            IMPORTED_LOCATION "${NUI_CEF_RUNTIME_DIR}/libcef.so"
            IMPORTED_SONAME "libcef.so"
    )
endif()

# Opt-in target for applications that use CEF types themselves (Nui::Cef client hooks).
add_library(nui-cef INTERFACE)
add_library(Nui::cef ALIAS nui-cef)
target_link_libraries(nui-cef INTERFACE nui-cef-headers libcef_dll_wrapper)
target_compile_definitions(nui-cef INTERFACE NUI_ENGINE_CEF)
