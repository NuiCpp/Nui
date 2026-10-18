# nui_configure_application(TARGET <target> [NAME <display name>] [IDENTIFIER <reverse dns id>] [VERSION <version>]
#                           [INFO_PLIST <template>] [ICON <icns file>] [RESOURCES <file or directory>...])
#
# Application properties of a Nui executable. With the CEF engine on macOS it turns the executable into an app bundle
# with the CEF framework and the helper apps, which CEF needs to run, also in the build tree. Call it in the directory
# that defines the target. Otherwise it only records the properties.

set(NUI_CODESIGN_IDENTITY "-" CACHE STRING "macOS: identity for codesign while installing, - signs ad hoc")
option(NUI_CODESIGN_HARDENED_RUNTIME "macOS: sign with the hardened runtime (needed for notarization)" OFF)

function(nui_configure_application)
    cmake_parse_arguments(PARSE_ARGV 0 NUI_APP "" "TARGET;NAME;IDENTIFIER;VERSION;INFO_PLIST;ICON" "RESOURCES")
    if (NUI_APP_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "nui_configure_application: unknown arguments ${NUI_APP_UNPARSED_ARGUMENTS}")
    endif()
    if (NOT NUI_APP_TARGET)
        message(FATAL_ERROR "nui_configure_application: TARGET is required")
    endif()
    # Arguments after RESOURCES all count as resources, which may also be generated later. A missing one written like
    # a keyword is a misspelled keyword.
    foreach (resource ${NUI_APP_RESOURCES})
        if (resource MATCHES "^[A-Z][A-Z0-9_]*$" AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${resource}")
            message(FATAL_ERROR "nui_configure_application: unknown argument ${resource} (resource does not exist)")
        endif()
    endforeach()
    set(target ${NUI_APP_TARGET})
    if (NOT NUI_APP_NAME)
        set(NUI_APP_NAME ${target})
    endif()
    if (NOT NUI_APP_IDENTIFIER)
        string(MAKE_C_IDENTIFIER "${target}" identifierName)
        set(NUI_APP_IDENTIFIER "org.nuicpp.${identifierName}")
    endif()
    if (NOT NUI_APP_VERSION)
        set(NUI_APP_VERSION "${PROJECT_VERSION}")
    endif()
    if (NOT NUI_APP_VERSION)
        set(NUI_APP_VERSION "0.0.0")
    endif()
    set_target_properties(${target}
        PROPERTIES
            NUI_APPLICATION_NAME "${NUI_APP_NAME}"
            NUI_APPLICATION_IDENTIFIER "${NUI_APP_IDENTIFIER}"
            NUI_APPLICATION_VERSION "${NUI_APP_VERSION}"
    )

    if (NOT APPLE OR NOT NUI_BROWSER_ENGINE STREQUAL "cef")
        return()
    endif()
    _nui_configure_macos_bundle()
endfunction()

macro(_nui_configure_macos_bundle)
    set(bundleDirectory "${CMAKE_CURRENT_BINARY_DIR}/nui-bundle/${target}")
    if (CMAKE_OSX_DEPLOYMENT_TARGET)
        set(NUI_BUNDLE_MINIMUM_SYSTEM "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
        set(NUI_BUNDLE_MINIMUM_SYSTEM "13.0")
    endif()

    if (NUI_APP_INFO_PLIST)
        set(infoPlist "${NUI_APP_INFO_PLIST}")
    else()
        set(infoPlist "${bundleDirectory}/Info.plist.in")
        configure_file("${NUI_SOURCE_DIRECTORY}/cmake/backend/mac/Info.plist.in" "${infoPlist}" @ONLY)
    endif()

    set(iconFile "")
    if (NUI_APP_ICON)
        get_filename_component(iconFile "${NUI_APP_ICON}" NAME)
        target_sources(${target} PRIVATE "${NUI_APP_ICON}")
        set_source_files_properties("${NUI_APP_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    endif()

    set_target_properties(${target}
        PROPERTIES
            MACOSX_BUNDLE ON
            MACOSX_BUNDLE_INFO_PLIST "${infoPlist}"
            MACOSX_BUNDLE_BUNDLE_NAME "${NUI_APP_NAME}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${NUI_APP_IDENTIFIER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${NUI_APP_VERSION}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${NUI_APP_VERSION}"
            MACOSX_BUNDLE_ICON_FILE "${iconFile}"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    )

    set(contents "$<TARGET_BUNDLE_CONTENT_DIR:${target}>")
    set(sourceFramework "${NUI_CEF_LIBRARY_DIR}/Chromium Embedded Framework.framework")
    set(framework "${contents}/Frameworks/Chromium Embedded Framework.framework")
    set(commands
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${framework}/Versions"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${sourceFramework}" "${framework}/Versions/A"
    )
    foreach (link "Versions/Current;A" "Chromium Embedded Framework;Versions/Current/Chromium Embedded Framework"
                  "Libraries;Versions/Current/Libraries" "Resources;Versions/Current/Resources")
        list(GET link 0 linkName)
        list(GET link 1 linkTarget)
        list(APPEND commands
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${framework}/${linkName}"
            COMMAND "${CMAKE_COMMAND}" -E create_symlink "${linkTarget}" "${framework}/${linkName}"
        )
    endforeach()

    # The helper variants Chromium starts on macOS, all of them run the one nui-cef-helper executable.
    set(helperSuffixes "" " (Alerts)" " (GPU)" " (Plugin)" " (Renderer)")
    set(index 0)
    foreach (suffix IN LISTS helperSuffixes)
        string(REGEX REPLACE "[ ()]" "" identifierSuffix "${suffix}")
        string(TOLOWER "${identifierSuffix}" identifierSuffix)
        set(NUI_HELPER_IDENTIFIER "${NUI_APP_IDENTIFIER}.helper")
        if (identifierSuffix)
            string(APPEND NUI_HELPER_IDENTIFIER ".${identifierSuffix}")
        endif()
        set(NUI_HELPER_EXECUTABLE "$<TARGET_FILE_NAME:${target}> Helper${suffix}")
        set(NUI_HELPER_VERSION "${NUI_APP_VERSION}")
        configure_file("${NUI_SOURCE_DIRECTORY}/cmake/backend/mac/helper-Info.plist.in" "${bundleDirectory}/helper${index}.plist.in" @ONLY)
        file(GENERATE OUTPUT "${bundleDirectory}/$<CONFIG>/helper${index}.plist" INPUT "${bundleDirectory}/helper${index}.plist.in")
        set(helperContents "${contents}/Frameworks/${NUI_HELPER_EXECUTABLE}.app/Contents")
        list(APPEND commands
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${helperContents}/MacOS"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:nui-cef-helper>" "${helperContents}/MacOS/${NUI_HELPER_EXECUTABLE}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${bundleDirectory}/$<CONFIG>/helper${index}.plist" "${helperContents}/Info.plist"
        )
        math(EXPR index "${index} + 1")
    endforeach()

    foreach (resource ${NUI_APP_RESOURCES})
        get_filename_component(resourceName "${resource}" NAME)
        if (IS_DIRECTORY "${resource}")
            list(APPEND commands COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${resource}" "${contents}/Resources/${resourceName}")
        else()
            list(APPEND commands COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${resource}" "${contents}/Resources/${resourceName}")
        endif()
    endforeach()

    add_custom_target(${target}-nui-bundle
        ${commands}
        DEPENDS nui-cef-helper
        COMMENT "Updating the CEF framework and helpers of ${target}"
        VERBATIM
    )
    add_dependencies(${target} ${target}-nui-bundle)
endmacro()

# Installs app bundles and signs them again from the inside out, since installing may change the main executable.
function(_nui_install_macos targets destination componentArguments)
    install(TARGETS ${targets} BUNDLE DESTINATION "${destination}" ${componentArguments})

    set(NUI_ENTITLEMENTS_EXTRA "")
    if (NUI_CODESIGN_IDENTITY STREQUAL "-")
        # Ad hoc signatures have no team id, the framework only loads without library validation then.
        set(NUI_ENTITLEMENTS_EXTRA "    <key>com.apple.security.cs.disable-library-validation</key><true/>\n")
    endif()
    set(entitlements "${CMAKE_BINARY_DIR}/nui-bundle/entitlements.plist")
    configure_file("${NUI_SOURCE_DIRECTORY}/cmake/backend/mac/entitlements.plist.in" "${entitlements}" @ONLY)

    set(signArguments --force --sign "${NUI_CODESIGN_IDENTITY}")
    if (NUI_CODESIGN_HARDENED_RUNTIME)
        list(APPEND signArguments --options runtime --entitlements "${entitlements}")
    endif()
    if (NOT NUI_CODESIGN_IDENTITY STREQUAL "-")
        list(APPEND signArguments --timestamp)
    endif()
    # Identities ("Developer ID Application: Name (TEAM)") and paths contain spaces, every argument is quoted.
    set(quotedSignArguments "")
    foreach (argument IN LISTS signArguments)
        string(APPEND quotedSignArguments " [==[${argument}]==]")
    endforeach()

    foreach (target ${targets})
        install(CODE "
            set(nuiApp \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${destination}/$<TARGET_BUNDLE_DIR_NAME:${target}>\")
            if (IS_ABSOLUTE \"${destination}\")
                set(nuiApp \"\$ENV{DESTDIR}${destination}/$<TARGET_BUNDLE_DIR_NAME:${target}>\")
            endif()
            set(nuiFramework \"\${nuiApp}/Contents/Frameworks/Chromium Embedded Framework.framework\")
            file(GLOB nuiLibraries \"\${nuiFramework}/Versions/A/Libraries/*.dylib\")
            file(GLOB nuiHelpers LIST_DIRECTORIES true \"\${nuiApp}/Contents/Frameworks/*.app\")
            message(STATUS \"Signing \${nuiApp}\")
            foreach (nuiItem \${nuiLibraries} \"\${nuiFramework}\")
                execute_process(COMMAND codesign --force --sign [==[${NUI_CODESIGN_IDENTITY}]==] \"\${nuiItem}\" COMMAND_ERROR_IS_FATAL ANY)
            endforeach()
            foreach (nuiItem \${nuiHelpers} \"\${nuiApp}\")
                execute_process(COMMAND codesign${quotedSignArguments} \"\${nuiItem}\" COMMAND_ERROR_IS_FATAL ANY)
            endforeach()
        " ${componentArguments})
    endforeach()
endfunction()
