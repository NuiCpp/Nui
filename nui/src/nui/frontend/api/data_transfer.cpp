#include <nui/frontend/api/data_transfer.hpp>

namespace Nui::WebApi
{
    DataTransfer::DataTransfer()
        : ValWrapper{Nui::val::global("DataTransfer").new_()}
    {}
    DataTransfer::DataTransfer(Nui::val dataTransfer)
        : ValWrapper{std::move(dataTransfer)}
    {}
    DropEffect DataTransfer::dropEffect() const
    {
        const auto effect = val_["dropEffect"].as<std::string>();
        return dropEffectFromString(effect).value_or(DropEffect::None);
    }

    EffectAllowed DataTransfer::effectAllowed() const
    {
        const auto effect = val_["effectAllowed"].as<std::string>();
        return effectAllowedFromString(effect).value_or(EffectAllowed::None);
    }

    FileList DataTransfer::files() const
    {
        return FileList{val_["files"]};
    }

    DataTransferItemList DataTransfer::items() const
    {
        return DataTransferItemList{val_["items"]};
    }

    std::vector<std::string> DataTransfer::types() const
    {
        std::vector<std::string> result;
        Nui::val typesVal = val_["types"];
        const auto length = typesVal["length"].as<int>();
        result.reserve(static_cast<std::size_t>(length));
        for (int i = 0; i < length; ++i)
            result.push_back(typesVal[i].as<std::string>());

        return result;
    }

    void DataTransfer::clearData()
    {
        val_.call<void>("clearData");
    }

    void DataTransfer::clearData(std::string const& format)
    {
        val_.call<void>("clearData", format);
    }

    std::string DataTransfer::getData(std::string const& format) const
    {
        return val_.call<Nui::val>("getData", format).as<std::string>();
    }

    void DataTransfer::setData(std::string const& format, std::string const& data) const
    {
        val_.call<void>("setData", format, data);
    }

    void DataTransfer::setDragImage(Nui::val imgElement, long xOffset, long yOffset) const
    {
        val_.call<void>("setDragImage", std::move(imgElement), xOffset, yOffset);
    }
}