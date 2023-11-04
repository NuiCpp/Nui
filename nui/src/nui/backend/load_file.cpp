#include "load_file.hpp"

#include <fstream>

std::optional<std::string> loadFile(std::filesystem::path const& file)
{
    std::ifstream reader{file, std::ios::binary};
    if (!reader)
        return std::nullopt;
    reader.seekg(0, std::ios::end);
    std::string content(static_cast<std::size_t>(reader.tellg()), '\0');
    reader.seekg(0, std::ios::beg);
    reader.read(content.data(), static_cast<std::streamsize>(content.size()));
    return content;
}