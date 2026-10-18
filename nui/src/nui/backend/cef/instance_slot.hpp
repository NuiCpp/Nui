#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace Nui::Detail::Cef
{
    /**
     * @brief A CEF cache directory that only this process uses, held by a file lock.
     *
     * Chromium allows one process per cache directory and hands later launches to the first process. A slot per
     * process keeps instances independent (debug and release builds, parallel tests, two launches of one binary), while
     * the first few slots stay persistent so the GPU and shader caches survive restarts.
     *
     * Layout: <root>/<cef version>/<host name>/<slot number>. Older version directories are removed once no process of
     * that version runs, slots from persistentSlots on are removed when they are free at the next start.
     */
    class InstanceSlot
    {
      public:
        static constexpr int persistentSlots = 4;

        /**
         * @brief Locks the first free slot below the root, creating directories as needed.
         *
         * @param root Per application cache directory, for example ~/.cache/<app>/cef.
         * @param version Version of the loaded CEF.
         * @return The slot, or an error message.
         */
        static std::optional<InstanceSlot> acquire(std::filesystem::path const& root, std::string const& version, std::string& error);

        std::filesystem::path const& directory() const
        {
            return directory_;
        }

        ~InstanceSlot();
        InstanceSlot(InstanceSlot&& other) noexcept;
        InstanceSlot& operator=(InstanceSlot&& other) noexcept;
        InstanceSlot(InstanceSlot const&) = delete;
        InstanceSlot& operator=(InstanceSlot const&) = delete;

      private:
        InstanceSlot(std::filesystem::path directory, int slotLock, int versionLock);

        std::filesystem::path directory_;
        int slotLock_{-1};
        int versionLock_{-1};
    };
}
