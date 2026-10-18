#include "instance_slot.hpp"

#include <fmt/format.h>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <system_error>
#include <utility>

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

        bool tryLock(int descriptor, int operation)
        {
            while (true)
            {
                if (::flock(descriptor, operation | LOCK_NB) == 0)
                    return true;
                if (errno != EINTR)
                    return false;
            }
        }

        void lockBlocking(int descriptor, int operation)
        {
            while (::flock(descriptor, operation) != 0 && errno == EINTR)
            {
            }
        }

        std::string hostName()
        {
            char name[256] = {};
            if (::gethostname(name, sizeof(name) - 1) != 0 || name[0] == '\0')
                return "localhost";
            return name;
        }

        bool isSlotName(std::string const& name)
        {
            return !name.empty() && name.find_first_not_of("0123456789") == std::string::npos;
        }

        // Removes the directories of other CEF versions that no running process uses.
        void pruneOtherVersions(std::filesystem::path const& root, std::string const& version)
        {
            std::error_code error;
            for (auto const& entry : std::filesystem::directory_iterator{root, error})
            {
                const auto name = entry.path().filename().string();
                if (!entry.is_directory(error) || name == version)
                    continue;
                const auto lock = openLockFile(root / (name + ".lock"));
                if (*lock < 0 || !tryLock(*lock, LOCK_EX))
                    continue;
                std::filesystem::remove_all(entry.path(), error);
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

        // A shared lock on the version marks the version directory as in use, it is never unlinked.
        auto versionLock = openLockFile(root / (version + ".lock"));
        if (*versionLock < 0)
        {
            error = fmt::format("Cannot open {}: {}", (root / (version + ".lock")).string(), std::strerror(errno));
            return std::nullopt;
        }
        lockBlocking(*versionLock, LOCK_SH);
        pruneOtherVersions(root, version);

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
        if (*tableLock < 0)
        {
            error = fmt::format("Cannot open the slot table lock: {}", std::strerror(errno));
            return std::nullopt;
        }
        lockBlocking(*tableLock, LOCK_EX);

        std::optional<InstanceSlot> chosen;
        for (int slot = 0; !chosen; ++slot)
        {
            const auto directory = hostDirectory / std::to_string(slot);
            std::filesystem::create_directories(directory, filesystemError);
            auto slotLock = openLockFile(directory / "nui.lock");
            if (*slotLock < 0)
            {
                error = fmt::format("Cannot open {}: {}", (directory / "nui.lock").string(), std::strerror(errno));
                return std::nullopt;
            }
            if (!tryLock(*slotLock, LOCK_EX))
                continue;
            chosen.emplace(InstanceSlot{directory, std::exchange(*slotLock, -1), std::exchange(*versionLock, -1)});
        }

        for (auto const& entry : std::filesystem::directory_iterator{hostDirectory, filesystemError})
        {
            const auto name = entry.path().filename().string();
            if (!isSlotName(name) || std::stoi(name) < persistentSlots || entry.path() == chosen->directory())
                continue;
            const auto slotLock = openLockFile(entry.path() / "nui.lock");
            if (*slotLock < 0 || !tryLock(*slotLock, LOCK_EX))
                continue;
            std::filesystem::remove_all(entry.path(), filesystemError);
        }
        return chosen;
    }
}
