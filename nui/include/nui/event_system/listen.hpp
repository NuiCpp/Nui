#pragma once

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>

#include <utility>
#include <memory>
#include <functional>

namespace Nui
{
    template <typename ValueT>
    void listen(EventContext& eventContext, Observed<ValueT> const& obs, std::function<bool(ValueT const&)> onEvent)
    {
        const auto eventId = eventContext.registerEvent(Event{
            [obs = Detail::CopyableObservedWrap{obs}, onEvent = std::move(onEvent)](auto) {
                return onEvent(obs.value());
            },
            []() {
                return true;
            }});
        obs.attachEvent(eventId);
    }

    template <typename ValueT>
    void listen(EventContext& eventContext, Observed<ValueT> const& obs, std::function<void(ValueT const&)> onEvent)
    {
        return listen(eventContext, obs, [onEvent = std::move(onEvent)](ValueT const& value) {
            onEvent(value);
            return true;
        });
    }

    template <typename ValueT, typename FunctionT>
    void listen(EventContext& eventContext, Observed<ValueT> const& obs, FunctionT onEvent)
    {
        return listen(eventContext, obs, std::function(std::move(onEvent)));
    }

    template <typename ValueT>
    void listen(
        EventContext& eventContext,
        std::shared_ptr<Observed<ValueT>> const& obs,
        std::function<bool(ValueT const&)> onEvent)
    {
        const auto eventId = eventContext.registerEvent(Event{
            [weak = std::weak_ptr<Observed<ValueT>>{obs}, onEvent = std::move(onEvent)](auto) {
                if (auto obs = weak.lock(); obs)
                    return onEvent(obs->value());
                return false;
            },
            [weak = std::weak_ptr<Observed<ValueT>>{obs}]() {
                return !weak.expired();
            }});
        obs->attachEvent(eventId);
    }

    template <typename ValueT>
    void listen(
        EventContext& eventContext,
        std::shared_ptr<Observed<ValueT>> const& obs,
        std::function<void(ValueT const&)> onEvent)
    {
        return listen(eventContext, obs, [onEvent = std::move(onEvent)](ValueT const& value) {
            onEvent(value);
            return true;
        });
    }

    template <typename ValueT, typename FunctionT>
    void listen(EventContext& eventContext, std::shared_ptr<Observed<ValueT>> const& obs, FunctionT onEvent)
    {
        return listen(eventContext, obs, std::function(std::move(onEvent)));
    }

    template <typename ValueT, typename FunctionT>
    void listen(std::shared_ptr<Observed<ValueT>> const& obs, FunctionT onEvent)
    {
        return listen(globalEventContext, obs, std::function(std::move(onEvent)));
    }

    template <typename ValueT, typename FunctionT>
    void listen(Observed<ValueT> const& obs, FunctionT onEvent)
    {
        return listen(globalEventContext, obs, std::function(std::move(onEvent)));
    }
}