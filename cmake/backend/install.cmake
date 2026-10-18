# nui_install(TARGETS <target>... [DESTINATION <dir>] [ENGINE_DESTINATION <dir>] [COMPONENT <component>]
#             [FLATPAK_ZYPAK])
#
# Installs Nui application executables for the selected browser engine:
#   native         install(TARGETS) only.
#   cef, Linux     the executables to DESTINATION (default ${CMAKE_INSTALL_BINDIR}), the CEF runtime and the helper to
#                  ENGINE_DESTINATION (default ${CMAKE_INSTALL_LIBDIR}/<name of the first target>), linked with a
#                  relative RPATH. libcef is stripped while installing (NUI_CEF_STRIP), locales can be limited
#                  (NUI_CEF_LOCALES). With NUI_CEF_SYSTEM only the executables and the helper are installed.
#   cef, macOS     the app bundles (see nui_configure_application) to DESTINATION, signed again after the copy.
#
# FLATPAK_ZYPAK (Linux, CEF): the executables go to ENGINE_DESTINATION and a small launcher with the executable's name
# to DESTINATION. It starts the executable through zypak-wrapper (org.electronjs.Electron2.BaseApp) with
# ZYPAK_CEF_LIBRARY_PATH set, which Chromium's sandbox needs inside Flatpak.

include(GNUInstallDirs)

option(NUI_CEF_STRIP "Strip libcef and the other CEF libraries while installing" ON)
set(NUI_CEF_LOCALES "" CACHE STRING "Locales of CEF to install (for example en-US;de), empty for all. en-US is always installed")
option(NUI_CEF_INSTALL_CHROME_SANDBOX "Install the setuid chrome-sandbox helper (needs root ownership and mode 4755)" OFF)
option(NUI_CEF_SWIFTSHADER "Install SwiftShader, the software renderer used without GPU" ON)

function(nui_install)
    cmake_parse_arguments(PARSE_ARGV 0 NUI_INSTALL "FLATPAK_ZYPAK" "DESTINATION;ENGINE_DESTINATION;COMPONENT" "TARGETS")
    if (NUI_INSTALL_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "nui_install: unknown arguments ${NUI_INSTALL_UNPARSED_ARGUMENTS}")
    endif()
    if (NOT NUI_INSTALL_TARGETS)
        message(FATAL_ERROR "nui_install: TARGETS is required")
    endif()
    foreach (target ${NUI_INSTALL_TARGETS})
        if (NOT TARGET ${target})
            message(FATAL_ERROR "nui_install: '${target}' is not a target (a misspelled keyword after TARGETS?)")
        endif()
    endforeach()
    if (NOT NUI_INSTALL_DESTINATION)
        if (APPLE AND NUI_BROWSER_ENGINE STREQUAL "cef")
            set(NUI_INSTALL_DESTINATION ".")
        else()
            set(NUI_INSTALL_DESTINATION "${CMAKE_INSTALL_BINDIR}")
        endif()
    endif()
    set(componentArguments "")
    if (NUI_INSTALL_COMPONENT)
        set(componentArguments COMPONENT "${NUI_INSTALL_COMPONENT}")
    endif()

    if (NOT NUI_BROWSER_ENGINE STREQUAL "cef")
        install(TARGETS ${NUI_INSTALL_TARGETS}
            RUNTIME DESTINATION "${NUI_INSTALL_DESTINATION}"
            BUNDLE DESTINATION "${NUI_INSTALL_DESTINATION}"
            ${componentArguments}
        )
        return()
    endif()

    if (APPLE)
        _nui_install_macos("${NUI_INSTALL_TARGETS}" "${NUI_INSTALL_DESTINATION}" "${componentArguments}")
        return()
    endif()

    list(GET NUI_INSTALL_TARGETS 0 firstTarget)
    if (NOT NUI_INSTALL_ENGINE_DESTINATION)
        get_target_property(outputName ${firstTarget} OUTPUT_NAME)
        if (NOT outputName)
            set(outputName ${firstTarget})
        endif()
        set(NUI_INSTALL_ENGINE_DESTINATION "${CMAKE_INSTALL_LIBDIR}/${outputName}")
    endif()

    # Relative path from the executables (or launchers) to the engine directory, for RPATH and the helper lookup. It
    # cannot be known at configure time when only one of them is absolute (the prefix can change at install time).
    if (IS_ABSOLUTE "${NUI_INSTALL_DESTINATION}" AND NOT IS_ABSOLUTE "${NUI_INSTALL_ENGINE_DESTINATION}" OR
        NOT IS_ABSOLUTE "${NUI_INSTALL_DESTINATION}" AND IS_ABSOLUTE "${NUI_INSTALL_ENGINE_DESTINATION}")
        message(FATAL_ERROR "nui_install: DESTINATION and ENGINE_DESTINATION must both be relative or both absolute")
    endif()

    set(launcherDestination "")
    if (NUI_INSTALL_FLATPAK_ZYPAK)
        _nui_install_normal_directory("${NUI_INSTALL_DESTINATION}" normalDestination)
        _nui_install_normal_directory("${NUI_INSTALL_ENGINE_DESTINATION}" normalEngineDestination)
        if (normalDestination STREQUAL normalEngineDestination)
            message(FATAL_ERROR
                "nui_install: FLATPAK_ZYPAK installs a launcher named like the executable to DESTINATION, DESTINATION "
                "must differ from ENGINE_DESTINATION")
        endif()
        set(launcherDestination "${NUI_INSTALL_DESTINATION}")
        set(NUI_INSTALL_DESTINATION "${NUI_INSTALL_ENGINE_DESTINATION}")
    endif()

    set(executableDirectory "${NUI_INSTALL_DESTINATION}")
    set(engineDirectory "${NUI_INSTALL_ENGINE_DESTINATION}")
    if (NOT IS_ABSOLUTE "${executableDirectory}")
        set(executableDirectory "/nui-prefix/${executableDirectory}")
        set(engineDirectory "/nui-prefix/${engineDirectory}")
    endif()
    file(RELATIVE_PATH engineRelative "${executableDirectory}" "${engineDirectory}")
    if (engineRelative STREQUAL "")
        set(engineRelative ".")
    endif()

    if (NUI_CEF_IS_SYSTEM)
        # The library's path on the target: without the sysroot of a cross build.
        _nui_install_normal_directory("${NUI_CEF_LIBRARY_DIR}" libcefRunPath)
        if (CMAKE_SYSROOT)
            _nui_install_normal_directory("${CMAKE_SYSROOT}" sysroot)
            cmake_path(IS_PREFIX sysroot "${libcefRunPath}" inSysroot)
            if (inSysroot)
                cmake_path(RELATIVE_PATH libcefRunPath BASE_DIRECTORY "${sysroot}")
                if (libcefRunPath STREQUAL ".")
                    set(libcefRunPath "")
                endif()
                set(libcefRunPath "/${libcefRunPath}")
            endif()
        endif()
        set(executableRunPath "${libcefRunPath}")
        set(helperRunPath "${libcefRunPath}")
    else()
        set(executableRunPath "$ORIGIN/${engineRelative}")
        set(helperRunPath "$ORIGIN")
    endif()

    foreach (target ${NUI_INSTALL_TARGETS})
        set_property(TARGET ${target} APPEND PROPERTY INSTALL_RPATH "${executableRunPath}")
        # The helper is not next to the system libcef, the executable carries its location.
        if (NUI_CEF_IS_SYSTEM)
            set(source "${CMAKE_BINARY_DIR}/nui-install/${target}_helper_path.cpp")
            file(GENERATE OUTPUT "${source}"
                CONTENT "extern \"C\" char const nuiCefHelperRelativePath[] = \"${engineRelative}/nui-cef-helper\";\n")
            target_sources(${target} PRIVATE "${source}")
        endif()
    endforeach()
    install(TARGETS ${NUI_INSTALL_TARGETS} RUNTIME DESTINATION "${NUI_INSTALL_DESTINATION}" ${componentArguments})
    if (NUI_INSTALL_FLATPAK_ZYPAK)
        set(libcefPath "")
        if (NUI_CEF_IS_SYSTEM)
            cmake_path(APPEND libcefRunPath libcef.so OUTPUT_VARIABLE libcefPath)
        endif()
        _nui_install_flatpak_launchers("${NUI_INSTALL_TARGETS}" "${launcherDestination}" "${NUI_INSTALL_ENGINE_DESTINATION}"
            "${libcefPath}" "${componentArguments}")
    endif()

    get_property(installedEngineDirectories GLOBAL PROPERTY NUI_CEF_INSTALLED_ENGINE_DIRECTORIES)
    if ("${NUI_INSTALL_ENGINE_DESTINATION}" IN_LIST installedEngineDirectories)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY NUI_CEF_INSTALLED_ENGINE_DIRECTORIES "${NUI_INSTALL_ENGINE_DESTINATION}")

    set_property(TARGET nui-cef-helper PROPERTY INSTALL_RPATH "${helperRunPath}")
    install(TARGETS nui-cef-helper RUNTIME DESTINATION "${NUI_INSTALL_ENGINE_DESTINATION}" ${componentArguments})

    if (NUI_CEF_IS_SYSTEM)
        return()
    endif()

    _nui_install_cef_runtime("${NUI_INSTALL_ENGINE_DESTINATION}" "${componentArguments}")
endfunction()

# A path without ".", ".." and trailing slashes, for comparisons.
function(_nui_install_normal_directory path outVariable)
    cmake_path(NORMAL_PATH path)
    string(REGEX REPLACE "(.)/+$" "\\1" path "${path}")
    set(${outVariable} "${path}" PARENT_SCOPE)
endfunction()

# Copies the CEF runtime files of the distribution (real files, never the build tree's links).
function(_nui_install_cef_runtime destination componentArguments)
    set(strippedLibraries libcef.so libvk_swiftshader.so libvulkan.so.1)
    foreach (file ${NUI_CEF_RUNTIME_FILES})
        get_filename_component(name "${file}" NAME)
        if (NOT NUI_CEF_SWIFTSHADER AND name MATCHES "^(libvk_swiftshader\\.so|vk_swiftshader_icd\\.json)$")
            continue()
        endif()
        if (name STREQUAL "locales")
            _nui_install_cef_locales("${file}" "${destination}" "${componentArguments}")
        elseif (name IN_LIST strippedLibraries AND NUI_CEF_STRIP AND CMAKE_STRIP)
            install(CODE "
                set(nuiTarget \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${destination}/${name}\")
                if (IS_ABSOLUTE \"${destination}\")
                    set(nuiTarget \"\$ENV{DESTDIR}${destination}/${name}\")
                endif()
                get_filename_component(nuiDirectory \"\${nuiTarget}\" DIRECTORY)
                file(MAKE_DIRECTORY \"\${nuiDirectory}\")
                message(STATUS \"Installing (stripped): \${nuiTarget}\")
                execute_process(
                    COMMAND \"${CMAKE_STRIP}\" --strip-all -o \"\${nuiTarget}\" \"${file}\"
                    RESULT_VARIABLE nuiStripResult)
                if (NOT nuiStripResult EQUAL 0)
                    message(WARNING \"Stripping ${name} failed, installing it unstripped\")
                    file(COPY_FILE \"${file}\" \"\${nuiTarget}\")
                endif()
                file(CHMOD \"\${nuiTarget}\" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
                list(APPEND CMAKE_INSTALL_MANIFEST_FILES \"\${nuiTarget}\")
            " ${componentArguments})
        elseif (name IN_LIST strippedLibraries)
            install(PROGRAMS "${file}" DESTINATION "${destination}" ${componentArguments})
        else()
            install(FILES "${file}" DESTINATION "${destination}" ${componentArguments})
        endif()
    endforeach()

    if (NUI_CEF_INSTALL_CHROME_SANDBOX)
        install(PROGRAMS "${NUI_CEF_LIBRARY_DIR}/chrome-sandbox"
            DESTINATION "${destination}"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
            ${componentArguments}
        )
    endif()
endfunction()

function(_nui_install_cef_locales directory destination componentArguments)
    if (NOT NUI_CEF_LOCALES)
        install(DIRECTORY "${directory}/" DESTINATION "${destination}/locales" ${componentArguments})
        return()
    endif()
    set(locales ${NUI_CEF_LOCALES})
    if (NOT "en-US" IN_LIST locales)
        list(APPEND locales en-US)
    endif()
    set(patterns "")
    foreach (locale ${locales})
        if (NOT EXISTS "${directory}/${locale}.pak")
            message(FATAL_ERROR "NUI_CEF_LOCALES: CEF has no locale '${locale}'")
        endif()
        list(APPEND patterns PATTERN "${locale}.pak" PATTERN "${locale}_*.pak")
    endforeach()
    install(DIRECTORY "${directory}/" DESTINATION "${destination}/locales" ${componentArguments} FILES_MATCHING ${patterns})
endfunction()

# One launcher per executable: sets ZYPAK_CEF_LIBRARY_PATH and execs zypak-wrapper with the real executable. libcefPath
# is the absolute path of a system libcef, empty for the one installed to the engine destination.
function(_nui_install_flatpak_launchers targets destination engineDestination libcefPath componentArguments)
    set(launcherDirectory "/nui-prefix/${destination}")
    set(engineDirectory "/nui-prefix/${engineDestination}")
    if (IS_ABSOLUTE "${destination}")
        set(launcherDirectory "${destination}")
        set(engineDirectory "${engineDestination}")
    endif()
    file(RELATIVE_PATH engineRelative "${launcherDirectory}" "${engineDirectory}")
    # Paths go into raw string literals, they may contain quotes and backslashes.
    if (libcefPath)
        set(libcefExpression "std::string{R\"nui(${libcefPath})nui\"}")
    else()
        set(libcefExpression "engine + \"/libcef.so\"")
    endif()
    foreach (target ${targets})
        set(source "${CMAKE_BINARY_DIR}/nui-install/${target}_flatpak_launcher.cpp")
        file(GENERATE OUTPUT "${source}" CONTENT "\
// Generated by nui_install(FLATPAK_ZYPAK): starts $<TARGET_FILE_NAME:${target}> through zypak.
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    char self[4096] = {};
    const auto length = ::readlink(\"/proc/self/exe\", self, sizeof(self) - 1);
    if (length <= 0)
    {
        std::perror(\"readlink /proc/self/exe\");
        return 127;
    }
    const std::string executable{self, static_cast<std::size_t>(length)};
    const auto engine = executable.substr(0, executable.rfind('/')) + R\"nui(/${engineRelative})nui\";
    ::setenv(\"ZYPAK_CEF_LIBRARY_PATH\", (${libcefExpression}).c_str(), 1);
    std::string wrapper{\"zypak-wrapper\"};
    std::string target = engine + \"/$<TARGET_FILE_NAME:${target}>\";
    std::vector<char*> arguments{wrapper.data(), target.data()};
    for (int index = 1; index < argc; ++index)
        arguments.push_back(argv[index]);
    arguments.push_back(nullptr);
    ::execvp(wrapper.c_str(), arguments.data());
    std::perror(\"zypak-wrapper\");
    return 127;
}
")
        add_executable(${target}-flatpak-launcher "${source}")
        set_target_properties(${target}-flatpak-launcher
            PROPERTIES
                OUTPUT_NAME "$<TARGET_FILE_BASE_NAME:${target}>"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/nui-flatpak-launcher$<0:>"
        )
        install(TARGETS ${target}-flatpak-launcher RUNTIME DESTINATION "${destination}" ${componentArguments})
    endforeach()
endfunction()
