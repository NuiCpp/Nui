#include <nui/frontend/api/drag_event.hpp>

namespace Nui::WebApi
{
    DragEvent::DragEvent(Nui::val dragEvent)
        : ValWrapper{std::move(dragEvent)}
    {}

    std::optional<DataTransfer> DragEvent::dataTransfer() const
    {
        if (!val_.hasOwnProperty("dataTransfer"))
            return std::nullopt;

        Nui::val dt = val_["dataTransfer"];
        if (dt.isUndefined() || dt.isNull())
            return std::nullopt;

        return DataTransfer{dt};
    }
}