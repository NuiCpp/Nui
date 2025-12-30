#include <nui/frontend/api/resize_observer_entry.hpp>

namespace Nui::WebApi
{
    ResizeObserverEntry::ResizeObserverEntry(Nui::val event)
        : ValWrapper{std::move(event)}
    {}

    std::vector<ResizeObserverEntry::BoxSize> ResizeObserverEntry::borderBoxSize() const
    {
        if (!val_.hasOwnProperty("borderBoxSize"))
            return {};

        std::vector<BoxSize> result;
        for (auto const& val : val_["borderBoxSize"])
        {
            BoxSize box;
            box.blockSize = val["blockSize"].as<double>();
            box.inlineSize = val["inlineSize"].as<double>();
            result.push_back(box);
        }
        return result;
    }

    std::vector<ResizeObserverEntry::BoxSize> ResizeObserverEntry::contentBoxSize() const
    {
        if (!val_.hasOwnProperty("contentBoxSize"))
            return {};

        std::vector<BoxSize> result;
        for (auto const& val : val_["contentBoxSize"])
        {
            BoxSize box;
            box.blockSize = val["blockSize"].as<double>();
            box.inlineSize = val["inlineSize"].as<double>();
            result.push_back(box);
        }
        return result;
    }

    DomRectReadOnly ResizeObserverEntry::contentRect() const
    {
        return DomRectReadOnly{val_["contentRect"]};
    }

    std::vector<ResizeObserverEntry::BoxSize> ResizeObserverEntry::devicePixelContentBoxSize() const
    {
        if (!val_.hasOwnProperty("devicePixelContentBoxSize"))
            return {};

        std::vector<BoxSize> result;
        for (auto const& val : val_["devicePixelContentBoxSize"])
        {
            BoxSize box;
            box.blockSize = val["blockSize"].as<double>();
            box.inlineSize = val["inlineSize"].as<double>();
            result.push_back(box);
        }
        return result;
    }

    Nui::val ResizeObserverEntry::target() const
    {
        return val_["target"];
    }
}