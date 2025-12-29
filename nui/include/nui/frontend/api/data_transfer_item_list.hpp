#pragma once

#include <nui/frontend/api/file.hpp>

#include <nui/frontend/val_wrapper.hpp>

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

        void remove(int index);

        void clear();
    };
}