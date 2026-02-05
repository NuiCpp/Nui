#pragma once

#include <nui/frontend/api/file.hpp>
#include <nui/frontend/api/data_transfer_item.hpp>

#include <nui/frontend/val_wrapper.hpp>

#include <string>

namespace Nui::WebApi
{
    class DataTransferItemList : public ValWrapper
    {
      public:
        explicit DataTransferItemList(Nui::val dataTransferItemList);

        int length() const;

        void add(File file);
        void add(std::string const& str);
        void add(std::string const& str, std::string const& mimeType);

        std::optional<DataTransferItem> operator[](int index) const;

        void remove(int index);

        void clear();
    };
}