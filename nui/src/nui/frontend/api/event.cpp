#include <nui/frontend/api/event.hpp>

namespace Nui::WebApi
{
    Event::Event(Nui::val event)
        : ValWrapper{std::move(event)}
    {}

    bool Event::bubbles() const
    {
        return val_["bubbles"].as<bool>();
    }
    bool Event::cancelable() const
    {
        return val_["cancelable"].as<bool>();
    }
    bool Event::composed() const
    {
        return val_["composed"].as<bool>();
    }
    Nui::val Event::currentTarget() const
    {
        return val_["currentTarget"];
    }
    bool Event::defaultPrevented() const
    {
        return val_["defaultPrevented"].as<bool>();
    }
    EventPhase Event::eventPhase() const
    {
        return static_cast<EventPhase>(val_["eventPhase"].as<int>());
    }
    bool Event::isTrusted() const
    {
        return val_["isTrusted"].as<bool>();
    }
    Nui::val Event::target() const
    {
        return val_["target"];
    }
    std::chrono::milliseconds Event::timeStamp() const
    {
        return std::chrono::milliseconds{static_cast<long long>(val_["timeStamp"].as<double>())};
    }
    std::string Event::type() const
    {
        return val_["type"].as<std::string>();
    }
    std::vector<Nui::val> Event::composedPath() const
    {
        auto pathArray = val_.call<Nui::val>("composedPath");
        std::vector<Nui::val> result;
        const auto length = pathArray["length"].as<int>();
        result.reserve(static_cast<std::size_t>(length));
        for (int i = 0; i < length; ++i)
            result.push_back(pathArray[i]);
        return result;
    }
    void Event::preventDefault() const
    {
        val_.call<void>("preventDefault");
    }
    void Event::stopImmediatePropagation() const
    {
        val_.call<void>("stopImmediatePropagation");
    }
    void Event::stopPropagation() const
    {
        val_.call<void>("stopPropagation");
    }
}