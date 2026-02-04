#include <nui/frontend/api/data_transfer_item.hpp>

#include <nui/frontend/utility/functions.hpp>

namespace Nui::WebApi
{
    DataTransferItem::DataTransferItem(Nui::val dataTransferItem)
        : ValWrapper{std::move(dataTransferItem)}
    {}
    DataTransferItem::Kind DataTransferItem::kind() const
    {
        const auto kindStr = val_["kind"].as<std::string>();
        if (kindStr == "string")
            return Kind::String;
        if (kindStr == "file")
            return Kind::File;
        return Kind::Unknown;
    }
    std::string DataTransferItem::type() const
    {
        return val_["type"].as<std::string>();
    }
    std::optional<File> DataTransferItem::getAsFile() const
    {
        auto fileVal = val_.call<Nui::val>("getAsFile");
        if (fileVal.isNull() || fileVal.isUndefined())
            return std::nullopt;
        return File{std::move(fileVal)};
    }
    Nui::val DataTransferItem::getAsStringCallback() const
    {
        return val_.call<Nui::val>("getAsString");
    }
    void DataTransferItem::getAsString(std::function<void(std::optional<std::string> const&)> callback) const
    {
        val_.call<void>(
            "getAsString",
            Nui::bind(
                [callback = std::move(callback)](Nui::val v) {
                    if (v.isNull() || v.isUndefined())
                    {
                        callback(std::nullopt);
                        return;
                    }
                    callback(v.as<std::string>());
                },
                std::placeholders::_1));
    }
    std::optional<FileSystemEntry> DataTransferItem::webkitGetAsEntry() const
    {
        auto entryVal = val_.call<Nui::val>("webkitGetAsEntry");
        if (!entryVal.isNull() && !entryVal.isUndefined())
        {
            return FileSystemEntry{std::move(entryVal)};
        }
        return std::nullopt;
    }
}