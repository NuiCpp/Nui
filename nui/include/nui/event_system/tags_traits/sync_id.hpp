#pragma once

#include <nui/event_system/observed_value.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace Nui
{
    namespace Detail
    {
        constexpr static std::size_t locationIdMaxLen = 128;
        constexpr static std::size_t maxLineDigits = 20;
        constexpr static int decimalBase = 10;

        // A fixed-size string usable as a C++20 non-type template parameter.
        // Encodes "linenumber_sanitized_filename" so the line number is never truncated
        // and the resulting RPC names are human-readable and identical across all
        // compilers / build trees regardless of __FILE__ expansion differences.
        struct LocationId
        {
            std::array<char, locationIdMaxLen> buf{};
            std::size_t len{0};

            constexpr LocationId() = default;

            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
            constexpr LocationId(std::string_view filename, int line) noexcept
            {
                std::size_t i = 0;

                // Write line number first so it is never truncated by a long filename
                if (line <= 0)
                {
                    if (i < locationIdMaxLen - 1)
                        buf[i++] = '0';
                }
                else
                {
                    std::array<char, maxLineDigits> digits{};
                    int dlen = 0;
                    int tmp = line;
                    while (tmp > 0 && std::cmp_less(dlen, maxLineDigits))
                    {
                        digits[static_cast<std::size_t>(dlen++)] =
                            static_cast<char>('0' + (tmp % decimalBase));
                        tmp /= decimalBase;
                    }
                    for (int d = dlen - 1; d >= 0 && i < locationIdMaxLen - 1; --d)
                        buf[i++] = digits[static_cast<std::size_t>(d)];
                }

                // Separator
                if (i < locationIdMaxLen - 1)
                    buf[i++] = '_';

                // Copy sanitized filename: keep alnum, '.', '_'; map anything else to '_'
                for (char c : filename)
                {
                    if (i >= locationIdMaxLen - 1)
                        break;
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '_')
                        buf[i++] = c;
                    else
                        buf[i++] = '_';
                }

                len = i;
            }
            // NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

            constexpr std::string_view view() const noexcept
            {
                return {buf.data(), len};
            }
            constexpr bool operator==(LocationId const&) const = default;
        };

        // Strip path — find last / or \ and return only the filename portion
        constexpr std::string_view filenameOnly(std::string_view path) noexcept
        {
            const auto pos = path.find_last_of("/\\");
            if (pos != std::string_view::npos)
                path.remove_prefix(pos + 1);
            return path;
        }

        constexpr LocationId makeLocationId(std::string_view file, int line) noexcept
        {
            return LocationId{file, line};
        }
    }

    template <Detail::LocationId Id>
    struct SyncId
    {
        // Keep a copy as a static member so string_view into it has stable storage.
        static constexpr Detail::LocationId storage = Id;
        static constexpr std::string_view syncId = {storage.buf.data(), storage.len};
    };

    template <typename T, typename Tags>
    constexpr std::string_view syncId(Observed<T, Tags> const&)
    {
        return Tags::syncId;
    }
}

#define NUI_SYNC_ID \
    ::Nui::SyncId<::Nui::Detail::makeLocationId(::Nui::Detail::filenameOnly(__FILE__), __LINE__)>

#define NUI_SYNCED_TAGS \
    struct \
    { \
        using SyncId = NUI_SYNC_ID; \
    }

#define NUI_SYNCHRONIZE \
    Nui::TagContainer< \
        ::Nui::SyncId<::Nui::Detail::makeLocationId(::Nui::Detail::filenameOnly(__FILE__), __LINE__)>>
