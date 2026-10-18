#include "instance_slot.hpp"

#include <fmt/format.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace Nui::Detail::Cef
{
    namespace
    {
        struct FileDescriptorCloser
        {
            void operator()(int* descriptor) const
            {
                if (*descriptor >= 0)
                    ::close(*descriptor);
                delete descriptor;
            }
        };
        using FileDescriptor = std::unique_ptr<int, FileDescriptorCloser>;

        FileDescriptor openLockFile(std::filesystem::path const& path)
        {
            return FileDescriptor{new int{::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600)}};
        }

        enum class LockResult
        {
            Locked,
            Busy,
            Failed,
        };

        LockResult tryLock(int descriptor, int operation)
        {
            while (true)
            {
                if (::flock(descriptor, operation | LOCK_NB) == 0)
                    return LockResult::Locked;
                if (errno == EWOULDBLOCK)
                    return LockResult::Busy;
                if (errno != EINTR)
                    return LockResult::Failed;
            }
        }

        bool lockBlocking(int descriptor, int operation)
        {
            while (::flock(descriptor, operation) != 0)
            {
                if (errno != EINTR)
                    return false;
            }
            return true;
        }

        /// The lock file may have been replaced (deleted by a cache cleaner) after it was opened, the lock is then
        /// worthless.
        bool lockedFileIsCurrent(int descriptor, std::filesystem::path const& path)
        {
            struct stat opened{};
            struct stat current{};
            return ::fstat(descriptor, &opened) == 0 && ::stat(path.c_str(), &current) == 0 &&
                opened.st_ino == current.st_ino && opened.st_dev == current.st_dev;
        }

        std::optional<std::vector<int>> parseVersion(std::string_view text)
        {
            std::vector<int> parts;
            while (!text.empty())
            {
                const auto dot = text.find('.');
                const auto part = text.substr(0, dot);
                int value = 0;
                const auto [end, error] = std::from_chars(part.data(), part.data() + part.size(), value);
                if (part.empty() || error != std::errc{} || end != part.data() + part.size())
                    return std::nullopt;
                parts.push_back(value);
                text = dot == std::string_view::npos ? std::string_view{} : text.substr(dot + 1);
            }
            if (parts.empty())
                return std::nullopt;
            return parts;
        }

        std::string hostName()
        {
            char name[256] = {};
            if (::gethostname(name, sizeof(name) - 1) != 0 || name[0] == '\0')
                return "localhost";
            return name;
        }

        /// Chromium's own profile lock, a link to "<host>-<pid>". It still names a running process when the slot's
        /// nui.lock was deleted while that process runs.
        bool chromiumOwnerAlive(std::filesystem::path const& directory)
        {
            std::error_code error;
            const auto target = std::filesystem::read_symlink(directory / "SingletonLock", error).string();
            const auto dash = target.rfind('-');
            if (error || dash == std::string::npos || target.substr(0, dash) != hostName())
                return false;
            const std::string_view pidText{target.data() + dash + 1, target.size() - dash - 1};
            int pid = 0;
            const auto [end, parseError] = std::from_chars(pidText.data(), pidText.data() + pidText.size(), pid);
            if (parseError != std::errc{} || end != pidText.data() + pidText.size() || pid <= 0 || pid == ::getpid())
                return false;
            return ::kill(pid, 0) == 0 || errno == EPERM;
        }

        bool anySlotOwnerAlive(std::filesystem::path const& versionDirectory)
        {
            std::error_code error;
            for (auto const& slot : std::filesystem::directory_iterator{versionDirectory / hostName(), error})
            {
                if (chromiumOwnerAlive(slot.path()))
                    return true;
            }
            return false;
        }

        std::optional<int> slotNumber(std::string const& name)
        {
            int value = 0;
            const auto [end, error] = std::from_chars(name.data(), name.data() + name.size(), value);
            if (name.empty() || error != std::errc{} || end != name.data() + name.size() || value < 0)
                return std::nullopt;
            return value;
        }

        // Removes the directories of older CEF versions that no running process uses.
        void pruneOlderVersions(std::filesystem::path const& root, std::string const& version)
        {
            const auto current = parseVersion(version);
            std::error_code error;
            for (auto const& entry : std::filesystem::directory_iterator{root, error})
            {
                const auto name = entry.path().filename().string();
                const auto other = parseVersion(name);
                if (!entry.is_directory(error) || !current || !other || !(*other < *current))
                    continue;
                const auto lockPath = root / (name + ".lock");
                const auto lock = openLockFile(lockPath);
                // Another pruner may have removed the file between open and lock, a process of that version may have
                // created a new one since.
                if (*lock < 0 || tryLock(*lock, LOCK_EX) != LockResult::Locked || !lockedFileIsCurrent(*lock, lockPath) ||
                    anySlotOwnerAlive(entry.path()))
                    continue;
                std::filesystem::remove_all(entry.path(), error);
                std::filesystem::remove(lockPath, error);
            }
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    InstanceSlot::InstanceSlot(std::filesystem::path directory, int slotLock, int versionLock)
        : directory_{std::move(directory)}
        , slotLock_{slotLock}
        , versionLock_{versionLock}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    InstanceSlot::~InstanceSlot()
    {
        if (slotLock_ >= 0)
            ::close(slotLock_);
        if (versionLock_ >= 0)
            ::close(versionLock_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    InstanceSlot::InstanceSlot(InstanceSlot&& other) noexcept
        : directory_{std::move(other.directory_)}
        , slotLock_{std::exchange(other.slotLock_, -1)}
        , versionLock_{std::exchange(other.versionLock_, -1)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    InstanceSlot& InstanceSlot::operator=(InstanceSlot&& other) noexcept
    {
        std::swap(directory_, other.directory_);
        std::swap(slotLock_, other.slotLock_);
        std::swap(versionLock_, other.versionLock_);
        return *this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<InstanceSlot>
    InstanceSlot::acquire(std::filesystem::path const& root, std::string const& version, std::string& error)
    {
        std::error_code filesystemError;
        std::filesystem::create_directories(root, filesystemError);
        if (filesystemError)
        {
            error = fmt::format("Cannot create {}: {}", root.string(), filesystemError.message());
            return std::nullopt;
        }

        // A shared lock on the version marks the version directory as in use. The pruner of an older version removes
        // its lock file too, so the lock is only valid if the file still is the one that was locked.
        const auto versionLockPath = root / (version + ".lock");
        FileDescriptor versionLock{new int{-1}};
        bool versionLocked = false;
        for (int attempt = 0; attempt != 8 && !versionLocked; ++attempt)
        {
            versionLock = openLockFile(versionLockPath);
            if (*versionLock < 0)
            {
                error = fmt::format("Cannot open {}: {}", versionLockPath.string(), std::strerror(errno));
                return std::nullopt;
            }
            if (!lockBlocking(*versionLock, LOCK_SH))
            {
                error = fmt::format("Cannot lock {}: {}", versionLockPath.string(), std::strerror(errno));
                return std::nullopt;
            }
            versionLocked = lockedFileIsCurrent(*versionLock, versionLockPath);
        }
        if (!versionLocked)
        {
            error = fmt::format("Cannot lock {}: the lock file keeps being replaced", versionLockPath.string());
            return std::nullopt;
        }
        pruneOlderVersions(root, version);

        const auto hostDirectory = root / version / hostName();
        std::filesystem::create_directories(hostDirectory, filesystemError);
        if (filesystemError)
        {
            error = fmt::format("Cannot create {}: {}", hostDirectory.string(), filesystemError.message());
            return std::nullopt;
        }

        // Slot selection and pruning happen under one exclusive table lock, so no starting process opens a slot lock
        // while its directory is removed.
        const auto tableLock = openLockFile(hostDirectory / "slots.lock");
        if (*tableLock < 0 || !lockBlocking(*tableLock, LOCK_EX))
        {
            error = fmt::format("Cannot lock the slot table in {}: {}", hostDirectory.string(), std::strerror(errno));
            return std::nullopt;
        }

        constexpr int maximumSlots = 1024;
        std::optional<InstanceSlot> chosen;
        for (int slot = 0; !chosen; ++slot)
        {
            if (slot == maximumSlots)
            {
                error = fmt::format("All {} cache slots in {} are in use", maximumSlots, hostDirectory.string());
                return std::nullopt;
            }
            const auto directory = hostDirectory / std::to_string(slot);
            std::filesystem::create_directories(directory, filesystemError);
            const auto lockPath = directory / "nui.lock";
            auto slotLock = openLockFile(lockPath);
            if (*slotLock < 0)
            {
                error = fmt::format("Cannot open {}: {}", lockPath.string(), std::strerror(errno));
                return std::nullopt;
            }
            const auto result = tryLock(*slotLock, LOCK_EX);
            if (result == LockResult::Failed)
            {
                error = fmt::format("Cannot lock {}: {}", lockPath.string(), std::strerror(errno));
                return std::nullopt;
            }
            if (result == LockResult::Busy || !lockedFileIsCurrent(*slotLock, lockPath) || chromiumOwnerAlive(directory))
                continue;
            chosen.emplace(InstanceSlot{directory, std::exchange(*slotLock, -1), std::exchange(*versionLock, -1)});
        }

        for (auto const& entry : std::filesystem::directory_iterator{hostDirectory, filesystemError})
        {
            const auto number = slotNumber(entry.path().filename().string());
            if (!number || *number < persistentSlots || entry.path() == chosen->directory())
                continue;
            const auto slotLock = openLockFile(entry.path() / "nui.lock");
            if (*slotLock < 0 || tryLock(*slotLock, LOCK_EX) != LockResult::Locked || chromiumOwnerAlive(entry.path()))
                continue;
            std::filesystem::remove_all(entry.path(), filesystemError);
        }
        return chosen;
    }
}
