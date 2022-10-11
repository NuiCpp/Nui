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
     *
     *
     * @param path
     * @return std::filesystem::path
     */
    std::filesystem::path resolvePath(std::filesystem::path const& path);
}