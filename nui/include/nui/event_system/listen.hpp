#pragma once

#include <utility>
#include <memory>
#include <functional>

template <typename EventContextT, typename ValueT>
void listenUnsafe(
    EventContextT& eventContext,
    Nui::ObservedValue<ValueT> const& obs,
    std::function<bool(ValueT const&)> onEvent)
{
    const auto eventId = eventContext.registerEvent(Nui::Event{
        [obs = Nui::Detail::CopyableObservedWrap{obs}, onEvent = std::move(onEvent)](auto eventId) {
            return onEvent(obs.get());
        },
        []() {
            return true;
        }});
    obs.attachEvent(eventId);
}

template <typename EventContextT, typename ValueT>
void listen(
    EventContextT& eventContext,
    std::shared_ptr<Nui::ObservedValue<ValueT>> const& obs,
    std::function<bool(ValueT const&)> onEvent)
{
    const auto id = eventContext.registerEvent(Nui::Event{
        [weak = std::weak_ptr<Nui::ObservedValue<ValueT>>{obs}, onEvent = std::move(onEvent)](auto eventId) {
            if (auto obs = weak.lock(); obs)
                return onEvent(obs->get());
            return false;
        },
        [weak = std::weak_ptr<Nui::ObservedValue<ValueT>>{obs}]() {
            return !weak.expired();
        }});
}