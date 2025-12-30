#include <nui/frontend/api/resize_observer.hpp>

#include <nui/frontend/utility/functions.hpp>

#include <nui/frontend/api/console.hpp>

namespace Nui::WebApi
{
    ResizeObserver::ResizeObserver(
        std::function<void(std::vector<ResizeObserverEntry> const&, ResizeObserver const&)> callback)
        : ValWrapper{Nui::val::global("ResizeObserver")
                         .new_(
                             Nui::bind(
                                 [this](Nui::val entriesVal, Nui::val) {
                                     Nui::WebApi::Console::log("ResizeObserver callback invoked.");
                                     std::vector<ResizeObserverEntry> entries;
                                     for (auto const& entryVal : entriesVal)
                                     {
                                         entries.emplace_back(entryVal);
                                     }
                                     callback_(entries, *this);
                                 },
                                 std::placeholders::_1,
                                 std::placeholders::_2))}
        , callback_{std::move(callback)}
    {}
    ResizeObserver::ResizeObserver(Nui::val event)
        : ValWrapper(std::move(event))
        , callback_{}
    {}
    void ResizeObserver::disconnect() const
    {
        val_.call<void>("disconnect");
    }
    void ResizeObserver::observe(Nui::val target) const
    {
        val_.call<void>("observe", target);
    }
    void ResizeObserver::observe(Nui::val target, ObserveOptions const& options) const
    {
        Nui::val optionsVal = Nui::val::object();
        optionsVal.set("box", options.box);
        val_.call<void>("observe", target, optionsVal);
    }
    void ResizeObserver::unobserve(Nui::val target) const
    {
        val_.call<void>("unobserve", target);
    }
}