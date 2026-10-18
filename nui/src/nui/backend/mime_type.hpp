#pragma once

#include <optional>
#include <string>

namespace Nui::Detail
{
    /**
     * @brief MIME type of a file extension (with the dot) for folder mappings. Covers the types browsers check strictly
     * (WebAssembly streaming, module scripts) and falls back to Roar's table.
     */
    std::optional<std::string> mimeTypeOfExtension(std::string const& extension);
}
