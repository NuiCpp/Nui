#pragma once

#include <nui/frontend/api/file.hpp>
#include <nui/frontend/val_wrapper.hpp>

#include <optional>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
     */
    class FileList : public ValWrapper
    {
      public:
        explicit FileList(Nui::val fileList);

        int length() const;

        std::optional<File> item(int index) const;
    };
}