#include <nui/frontend/api/file_list.hpp>

namespace Nui::WebApi
{
    FileList::FileList(Nui::val fileList)
        : ValWrapper{std::move(fileList)}
    {}
    int FileList::length() const
    {
        return val_["length"].as<int>();
    }
    std::optional<File> FileList::item(int index) const
    {
        if (index >= length())
            return std::nullopt;
        auto fileVal = val_.call<Nui::val>("item", index);
        if (fileVal.isUndefined() || fileVal.isNull())
            return std::nullopt;
        return File{std::move(fileVal)};
    }
}