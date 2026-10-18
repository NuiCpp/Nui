#include "sandbox_linux.hpp"

#include <fcntl.h>
#include <linux/capability.h>
#include <sched.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Nui::Detail::Cef
{
    namespace
    {
        // Only async-signal-safe calls, the child of a multi threaded process runs this.
        bool writeFile(char const* path, char const* text)
        {
            const int descriptor = ::open(path, O_WRONLY | O_CLOEXEC);
            if (descriptor < 0)
                return false;
            const auto length = std::strlen(text);
            const bool written = ::write(descriptor, text, length) == static_cast<ssize_t>(length);
            ::close(descriptor);
            return written;
        }

        void formatMap(char* buffer, std::size_t size, unsigned long id)
        {
            std::snprintf(buffer, size, "%lu %lu 1\n", id, id);
        }

        /**
         * @brief Chromium's Credentials::CanCreateProcessInNewUserNS: a child in a new user namespace must be able to
         * map its ids and, without capabilities, create a nested user namespace. A plain unshare is not enough, Ubuntu's
         * AppArmor restriction allows creating the namespace but denies the id mapping.
         */
        bool canCreateUserNamespaces()
        {
            const auto user = ::getuid();
            const auto group = ::getgid();
            char userMap[64];
            char groupMap[64];
            formatMap(userMap, sizeof(userMap), user);
            formatMap(groupMap, sizeof(groupMap), group);

            const auto child = static_cast<pid_t>(::syscall(SYS_clone, CLONE_NEWUSER | SIGCHLD, nullptr, nullptr, nullptr, nullptr));
            if (child < 0)
                return false;
            if (child == 0)
            {
                if (!writeFile("/proc/self/setgroups", "deny") && errno != ENOENT)
                    ::_exit(1);
                if (!writeFile("/proc/self/gid_map", groupMap) || !writeFile("/proc/self/uid_map", userMap))
                    ::_exit(1);
                __user_cap_header_struct header{.version = _LINUX_CAPABILITY_VERSION_3, .pid = 0};
                __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3] = {};
                if (::syscall(SYS_capset, &header, data) != 0)
                    ::_exit(1);
                ::_exit(::unshare(CLONE_NEWUSER) == 0 ? 0 : 1);
            }
            int status = -1;
            pid_t waited = -1;
            do
            {
                waited = ::waitpid(child, &status, 0);
            } while (waited < 0 && errno == EINTR);
            return waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }

        bool usableSetuidSandbox(std::filesystem::path const& path)
        {
            struct stat status{};
            if (::stat(path.c_str(), &status) != 0)
                return false;
            if (status.st_uid != 0 || (status.st_mode & S_ISUID) == 0 || (status.st_mode & S_IXOTH) == 0)
                return false;
            if (::access(path.c_str(), X_OK) != 0)
                return false;
            struct statvfs fileSystem{};
            return ::statvfs(path.c_str(), &fileSystem) == 0 && (fileSystem.f_flag & ST_NOSUID) == 0;
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    SandboxDecision probeSandbox(std::filesystem::path const& libcefDirectory)
    {
        if (std::filesystem::exists("/.flatpak-info"))
        {
            // zypak replaces the zygote sandbox with Flatpak's own sandbox. For CEF it needs to know the library.
            if (std::getenv("ZYPAK_BIN") == nullptr)
                return {.disable = true, .reason = "running in Flatpak without zypak"};
            auto const* zypakLibrary = std::getenv("ZYPAK_CEF_LIBRARY_PATH");
            std::error_code error;
            if (zypakLibrary == nullptr || !std::filesystem::equivalent(zypakLibrary, libcefDirectory / "libcef.so", error))
            {
                return {
                    .disable = true,
                    .reason = "zypak is active but ZYPAK_CEF_LIBRARY_PATH does not name the loaded " +
                        (libcefDirectory / "libcef.so").string(),
                };
            }
            return {};
        }
        if (::geteuid() == 0)
            return {.disable = true, .reason = "Chromium's sandbox does not run as root"};
        if (canCreateUserNamespaces())
            return {};
        if (auto const* develSandbox = std::getenv("CHROME_DEVEL_SANDBOX");
            develSandbox != nullptr && usableSetuidSandbox(develSandbox))
            return {};
        if (usableSetuidSandbox(libcefDirectory / "chrome-sandbox"))
            return {};
        return {
            .disable = true,
            .reason = "unprivileged user namespaces are not available (on Ubuntu 23.10 and later they need an AppArmor "
                      "profile) and there is no setuid chrome-sandbox",
        };
    }
}
