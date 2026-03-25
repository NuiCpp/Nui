#pragma once

#include <nui/event_system/observed_value.hpp>

#include <cstdint>
#include <array>
#include <string_view>

namespace Nui
{
    namespace Detail
    {
        constexpr static std::uint64_t fnv1aPrime = 1099511628211ULL;
        constexpr static std::uint64_t fnv1aOffsetBasis = 14695981039346656037ULL;

        constexpr std::uint64_t fnv1a(std::string_view str) noexcept
        {
            std::uint64_t hash = fnv1aOffsetBasis;
            for (auto c : str)
            {
                hash ^= static_cast<std::uint64_t>(c);
                hash *= fnv1aPrime;
            }
            return hash;
        }

        // Strip path — find last / or \ and return only the filename portion
        constexpr std::string_view filenameOnly(std::string_view path) noexcept
        {
            path.remove_prefix(path.find_last_of("/\\") + 1);
            return path;
        }

        constexpr std::uint64_t locationHash(std::string_view file, int line) noexcept
        {
            return (fnv1a(file) ^ static_cast<std::uint64_t>(line)) * fnv1aPrime;
        }
    }

    template <std::uint64_t Hash>
    struct SyncId
    {
        static constexpr std::uint64_t syncId = Hash;
    };

    template <typename T, typename Tags>
    constexpr std::uint64_t syncId(Observed<T, Tags> const&)
    {
        return Tags::syncId;
    }
}

#define NUI_SYNC_ID ::Nui::SyncId<::Nui::Detail::locationHash(__FILE__, __LINE__)>
#define NUI_SYNCED_TAGS \
    struct \
    { \
        using SyncId = NUI_SYNC_ID; \
    }

#define NUI_SYNCHRONIZE Nui::TagContainer<::Nui::SyncId<::Nui::Detail::locationHash(__FILE__, __LINE__)>>