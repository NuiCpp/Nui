#include <nui/backend/filesystem/special_paths.hpp>

#if defined(_WIN32)
#    include <shlobj.h>
#else
#    include <unistd.h>
#    include <sys/types.h>
#    include <pwd.h>
#endif

#include <numeric>
#include <iterator>
#include <algorithm>
#include <cctype>

namespace Nui
{
    namespace
    {
#if defined(_WIN32)
        std::filesystem::path getSpecialPath(int csidl)
        {
            char profilePath[MAX_PATH * 5];
            HRESULT result = SHGetFolderPath(NULL, csidl, NULL, 0, profilePath);
            if (SUCCEEDED(result))
                return {profilePath};
            else
                throw std::runtime_error("Could not get folder path - windows error code: " + std::to_string(result));
        }
        std::filesystem::path getAppData()
        {
            return getSpecialPath(CSIDL_APPDATA);
        }
        std::filesystem::path getDocuments()
        {
            return getSpecialPath(CSIDL_MYDOCUMENTS);
        }
        std::filesystem::path getLocalAppData()
        {
            return getSpecialPath(CSIDL_LOCAL_APPDATA);
        }
        std::filesystem::path getHome()
        {
            return getSpecialPath(CSIDL_PROFILE);
        }
        std::filesystem::path getTemp()
        {
            return getLocalAppData() / "Temp";
        }
#else
        std::filesystem::path getHome()
        {
            auto const* homedir = getenv("HOME");
            if (homedir != nullptr)
                return {homedir};

            struct passwd* pw = getpwuid(getuid());
            if (pw != nullptr)
                return {pw->pw_dir};
            return std::filesystem::path{};
        }
        std::filesystem::path getAppData()
        {
            return getHome();
        }
        std::filesystem::path getLocalAppData()
        {
            return getHome();
        }
        std::filesystem::path getTemp()
        {
            return "/tmp";
        }
        std::filesystem::path getXdgConfigHome()
        {
            auto const* xdgConfigHome = getenv("XDG_CONFIG_HOME");
            if (xdgConfigHome != nullptr)
                return {xdgConfigHome};
            return getHome() / ".config";
        }
        std::filesystem::path getXdgStateHome()
        {
            auto const* xdgStateHome = getenv("XDG_STATE_HOME");
            if (xdgStateHome != nullptr)
                return {xdgStateHome};
            return getHome() / ".local/state";
        }
        std::filesystem::path getXdgDataHome()
        {
            auto const* xdgDataHome = getenv("XDG_DATA_HOME");
            if (xdgDataHome != nullptr)
                return {xdgDataHome};
            return getHome() / ".local/share";
        }
#endif
    }

    std::filesystem::path resolvePath(std::filesystem::path const& unresolvedPath)
    {
        if (unresolvedPath.empty())
            return unresolvedPath;

        auto path = unresolvedPath;
        auto lowerPath = path.string();
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        auto tryAndMap = [&lowerPath, &path, &unresolvedPath](auto needle, std::filesystem::path (*func)()) {
            if (lowerPath.starts_with(needle))
            {
                path = std::accumulate(
                    std::next(std::begin(unresolvedPath)),
                    std::end(unresolvedPath),
                    func(),
                    [](auto accumulated, auto const& toBeAdded) {
                        return std::move(accumulated) /= toBeAdded;
                    });
                return true;
            }
            return false;
        };

        if (tryAndMap("~", getHome))
            return path;
        if (tryAndMap("%userprofile%", getHome))
            return path;
        if (tryAndMap("%appdata%", getAppData))
            return path;
        if (tryAndMap("%localappdata%", getLocalAppData))
            return path;
        if (tryAndMap("%temp%", getTemp))
            return path;
#ifdef _WIN32
        if (tryAndMap("%config_home%", getAppData))
            return path;
        if (tryAndMap("%config_home2%", getDocuments))
            return path;
        if (tryAndMap("%config_home3%", getHome))
            return path;
        if (tryAndMap("%state_home%", getAppData))
            return path;
        if (tryAndMap("%state_home2%", getDocuments))
            return path;
        if (tryAndMap("%state_home3%", getHome))
            return path;
        if (tryAndMap("%data_home%", getAppData))
            return path;
        if (tryAndMap("%data_home2%", getDocuments))
            return path;
        if (tryAndMap("%data_home3%", getHome))
            return path;
#else
        if (tryAndMap("%config_home%", getXdgConfigHome))
            return path;
        if (tryAndMap("%config_home2%", getXdgConfigHome))
            return path;
        if (tryAndMap("%config_home3%", getXdgConfigHome))
            return path;
        if (tryAndMap("%state_home%", getXdgStateHome))
            return path;
        if (tryAndMap("%state_home2%", getXdgStateHome))
            return path;
        if (tryAndMap("%state_home3%", getXdgStateHome))
            return path;
        if (tryAndMap("%data_home%", getXdgDataHome))
            return path;
        if (tryAndMap("%data_home2%", getXdgDataHome))
            return path;
        if (tryAndMap("%data_home3%", getXdgDataHome))
            return path;
#endif

        return path;
    }
}