# nui_install(TARGETS <target>... [DESTINATION <dir>] [ENGINE_DESTINATION <dir>] [COMPONENT <component>])
#
# Installs Nui application executables for the selected browser engine:
#   native         install(TARGETS) only.
#   cef, Linux     the executables to DESTINATION (default ${CMAKE_INSTALL_BINDIR}), the CEF runtime and the helper to
#                  ENGINE_DESTINATION (default ${CMAKE_INSTALL_LIBDIR}/<name of the first target>), linked with a
#                  relative RPATH. libcef is stripped while installing (NUI_CEF_STRIP), locales can be limited
#                  (NUI_CEF_LOCALES). With NUI_CEF_SYSTEM only the executables and the helper are installed.
#   cef, macOS     the app bundles (see nui_configure_application) to DESTINATION, signed again after the copy.

include(GNUInstallDirs)

option(NUI_CEF_STRIP "Strip libcef and the other CEF libraries while installing" ON)
set(NUI_CEF_LOCALES "" CACHE STRING "Locales of CEF to install (for example en-US;de), empty for all. en-US is always installed")
option(NUI_CEF_INSTALL_CHROME_SANDBOX "Install the setuid chrome-sandbox helper (needs root ownership and mode 4755)" OFF)
option(NUI_CEF_SWIFTSHADER "Install SwiftShader, the software renderer used without GPU" ON)

function(nui_install)
    cmake_parse_arguments(PARSE_ARGV 0 NUI_INSTALL "" "DESTINATION;ENGINE_DESTINATION;COMPONENT" "TARGETS")
    if (NOT NUI_INSTALL_TARGETS)
        message(FATAL_ERROR "nui_install: TARGETS is required")
    endif()
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

    # Relative path from the executables to the engine directory, for RPATH and the helper lookup.
    set(executableDirectory "${NUI_INSTALL_DESTINATION}")
    set(engineDirectory "${NUI_INSTALL_ENGINE_DESTINATION}")
    if (NOT IS_ABSOLUTE "${executableDirectory}")
        set(executableDirectory "/nui-prefix/${executableDirectory}")
    endif()
    if (NOT IS_ABSOLUTE "${engineDirectory}")
        set(engineDirectory "/nui-prefix/${engineDirectory}")
    endif()
    file(RELATIVE_PATH engineRelative "${executableDirectory}" "${engineDirectory}")
    if (engineRelative STREQUAL "")
        set(engineRelative ".")
    endif()

    if (NUI_CEF_IS_SYSTEM)
        set(libcefRunPath "${NUI_CEF_LIBRARY_DIR}")
        if (CMAKE_SYSROOT)
            string(REPLACE "${CMAKE_SYSROOT}" "" libcefRunPath "${libcefRunPath}")
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
