# Downloads the selected CEF distribution (the pinned one or NUI_CEF_VERSION_OVERRIDE) ahead of an offline build.
#
#   cmake [-DNUI_CEF_PLATFORM=linux64] [-DNUI_CEF_DOWNLOAD_DIR=<dir>] [-DNUI_CEF_EXTRACT=ON]
#         [-DNUI_CEF_VERSION_OVERRIDE=<version> -DNUI_CEF_SHA256_OVERRIDE=<sha256>] -P cef_fetch.cmake
#
# Prints the archive path (and the extracted root with NUI_CEF_EXTRACT). Pass the archive to the build as
# NUI_CEF_ARCHIVE, or the extracted root as NUI_CEF_ROOT, together with NUI_FETCH_CEF=OFF.

cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/cef_version.cmake")

if (NOT NUI_CEF_PLATFORM)
    # Script mode leaves CMAKE_HOST_SYSTEM_PROCESSOR empty.
    set(hostProcessor "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    if (NOT hostProcessor)
        cmake_host_system_information(RESULT hostProcessor QUERY OS_PLATFORM)
    endif()
    nui_cef_platform_for("${CMAKE_HOST_SYSTEM_NAME}" "${hostProcessor}" NUI_CEF_PLATFORM)
endif()
if (NOT NUI_CEF_DOWNLOAD_DIR)
    nui_cef_default_download_directory(NUI_CEF_DOWNLOAD_DIR)
endif()
get_filename_component(NUI_CEF_DOWNLOAD_DIR "${NUI_CEF_DOWNLOAD_DIR}" ABSOLUTE)

nui_cef_download(${NUI_CEF_PLATFORM} "${NUI_CEF_DOWNLOAD_DIR}" archive)
message(STATUS "NUI_CEF_ARCHIVE=${archive}")
if (NUI_CEF_EXTRACT)
    nui_cef_extract("${archive}" "${NUI_CEF_DOWNLOAD_DIR}" root)
    message(STATUS "NUI_CEF_ROOT=${root}")
endif()
