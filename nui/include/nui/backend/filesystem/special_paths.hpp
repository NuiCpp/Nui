#pragma once

#include <filesystem>

namespace Nui
{
    /**
     * @brief Will replace prefixes like "~" and "%appdata%" with actual directories on linux and windows.
     *
     * The percent values are case insensitive.
     * ~ Linux: home. Windows: home
     * %userprofile% Linux: home. Windows: home
     * %appdata% Linux: home. Windows: CSIDL_APPDATA
     * %localappdata% Linux: home. Windows: CSIDL_APPDATA_LOCAL
     * %temp% Linux: /tmp. Windows: ${USER}\AppData\Local\Temp
     * %config_home% Linux: $XDG_CONFIG_HOME. Windows: CSIDL_APPDATA
     * %config_home2% Linux: $XDG_CONFIG_HOME. Windows: CSIDL_MYDOCUMENTS
     * %config_home3% Linux: $XDG_CONFIG_HOME. Windows: home
     * %state_home% Linux: $XDG_CONFIG_HOME. Windows: CSIDL_APPDATA
     * %state_home2% Linux: $XDG_CONFIG_HOME. Windows: CSIDL_MYDOCUMENTS
     * %state_home3% Linux: $XDG_CONFIG_HOME. Windows: home
     * %data_home% Linux: $XDG_DATA_HOME. Windows: CSIDL_APPDATA
     * %data_home2% Linux: $XDG_DATA_HOME. Windows: CSIDL_MYDOCUMENTS
     * %data_home3% Linux: $XDG_DATA_HOME. Windows: home
     *
     *
     * @param path
     * @return std::filesystem::path
     */
    std::filesystem::path resolvePath(std::filesystem::path const& path);
}