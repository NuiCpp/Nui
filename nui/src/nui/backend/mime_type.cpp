#include "mime_type.hpp"

#include <roar/mime_type.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <utility>

namespace Nui::Detail
{
    std::optional<std::string> mimeTypeOfExtension(std::string const& extension)
    {
        constexpr std::array<std::pair<std::string_view, std::string_view>, 11> types{{
            {".wasm", "application/wasm"},
            {".mjs", "text/javascript"},
            {".js", "text/javascript"},
            {".map", "application/json"},
            {".woff2", "font/woff2"},
            {".woff", "font/woff"},
            {".vtt", "text/vtt"},
            {".mp3", "audio/mpeg"},
            {".avif", "image/avif"},
            {".webmanifest", "application/manifest+json"},
            {".svg", "image/svg+xml"},
        }};
        std::string lower = extension;
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        if (const auto iter = std::ranges::find(types, lower, &std::pair<std::string_view, std::string_view>::first);
            iter != types.end())
            return std::string{iter->second};
        return Roar::extensionToMime(extension);
    }
}
