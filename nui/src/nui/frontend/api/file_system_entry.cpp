#include <nui/frontend/api/file_system_entry.hpp>

namespace Nui::WebApi
{
    FileSystemEntry::FileSystemEntry(Nui::val fileSystemEntry)
        : ValWrapper{std::move(fileSystemEntry)}
    {}
    Nui::val FileSystemEntry::filesystem() const
    {
        return val_["filesystem"];
    }
    std::string FileSystemEntry::fullPath() const
    {
        return val_["fullPath"].as<std::string>();
    }
    bool FileSystemEntry::isFile() const
    {
        return val_["isFile"].as<bool>();
    }
    bool FileSystemEntry::isDirectory() const
    {
        return val_["isDirectory"].as<bool>();
    }
    std::string FileSystemEntry::name() const
    {
        return val_["name"].as<std::string>();
    }
}