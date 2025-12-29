#pragma once

#include <nui/frontend/api/data_transfer.hpp>
#include <nui/frontend/val_wrapper.hpp>

#include <optional>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
     */
    class DragEvent : public ValWrapper
    {
      public:
        explicit DragEvent(Nui::val dragEvent);

        std::optional<DataTransfer> dataTransfer() const;
    };
}