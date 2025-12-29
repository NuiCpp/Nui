#include <nui/frontend/api/data_transfer_item_list.hpp>

namespace Nui::WebApi
{
    DataTransferItemList::DataTransferItemList(Nui::val dataTransferItemList)
        : ValWrapper{std::move(dataTransferItemList)}
    {}
    int DataTransferItemList::length() const
    {
        return val_["length"].as<int>();
    }
    void DataTransferItemList::add(File file)
    {
        val_.call<void>("add", file.val());
    }
    void DataTransferItemList::add(std::string const& str)
    {
        val_.call<void>("add", str);
    }
    void DataTransferItemList::add(std::string const& str, std::string const& mimeType)
    {
        val_.call<void>("add", str, mimeType);
    }
    void DataTransferItemList::remove(int index)
    {
        val_.call<void>("remove", index);
    }
    void DataTransferItemList::clear()
    {
        val_.call<void>("clear");
    }
}