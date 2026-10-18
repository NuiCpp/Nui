# Downloads the pinned CEF distribution ahead of an offline build.
#
#   cmake [-DNUI_CEF_PLATFORM=linux64] [-DNUI_CEF_DOWNLOAD_DIR=<dir>] [-DNUI_CEF_EXTRACT=ON] -P cef_fetch.cmake
#
# Prints the archive path (and the extracted root with NUI_CEF_EXTRACT). Pass the archive to the build as
# NUI_CEF_ARCHIVE, or the extracted root as NUI_CEF_ROOT, together with NUI_FETCH_CEF=OFF.

cmake_minimum_required(VERSION 3.28)

include("${CMAKE_CURRENT_LIST_DIR}/cef_version.cmake")

if (NOT NUI_CEF_PLATFORM)
    nui_cef_platform_for("${CMAKE_HOST_SYSTEM_NAME}" "${CMAKE_HOST_SYSTEM_PROCESSOR}" NUI_CEF_PLATFORM)
endif()
if (NOT NUI_CEF_DOWNLOAD_DIR)
    nui_cef_default_download_directory(NUI_CEF_DOWNLOAD_DIR)
endif()

nui_cef_download(${NUI_CEF_PLATFORM} "${NUI_CEF_DOWNLOAD_DIR}" archive)
message(STATUS "NUI_CEF_ARCHIVE=${archive}")
if (NUI_CEF_EXTRACT)
    nui_cef_extract("${archive}" "${NUI_CEF_DOWNLOAD_DIR}" root)
    message(STATUS "NUI_CEF_ROOT=${root}")
endif()
