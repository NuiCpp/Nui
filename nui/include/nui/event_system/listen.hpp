#pragma once

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>

#include <utility>
#include <memory>
#include <functional>
#include <type_traits>
#include <variant>

namespace Nui
{
    /**
     * @brief This class keeps track of an event registered to an observed value and removes it on destruction.
     */
    template <typename ObservedType>
    struct ListenRemover
    {
      public:
        using ObservedMemberType =
            std::conditional_t<IsSharedObserved<ObservedType>, ObservedType, ObservedType const*>;

        ListenRemover()
            : id_{EventContext::invalidEventId}
            , obs_{nullptr}
        {}
        ListenRemover(EventContext::EventIdType id, ObservedType const& obs)
            : id_{id}
            , obs_{std::get<ObservedMemberType>(
                  [&obs]() -> std::variant<ObservedType const*, std::shared_ptr<ObservedType>> {
                      if constexpr (IsSharedObserved<ObservedType>)
                          return obs;
                      else
                          return &obs;
                  }())}
        {}
        ~ListenRemover()
        {
            removeEvent();
        }
        ListenRemover(ListenRemover const&) = delete;
        ListenRemover& operator=(ListenRemover const&) = delete;
        ListenRemover(ListenRemover&& other) noexcept
            : id_{std::exchange(other.id_, EventContext::invalidEventId)}
            , obs_{std::exchange(other.obs_, nullptr)}
        {}
        ListenRemover& operator=(ListenRemover&& other) noexcept
        {
            if (this != &other)
            {
                removeEvent();

                id_ = std::exchange(other.id_, EventContext::invalidEventId);
                obs_ = std::exchange(other.obs_, nullptr);
            }
            return *this;
        }

        /**
         * @brief Remove the event from the event context and detach it from the observed value.
         */
        void removeEvent()
        {
            if (id_ != EventContext::invalidEventId && obs_ != nullptr)
            {
                obs_->detachEvent(id_);
                obs_->eventContext().removeEvent(id_);
                id_ = EventContext::invalidEventId;
            }
        }

        /**
         * @brief Disarm the remover so that it won't remove the event on destruction.
         */
        void disarm()
        {
            id_ = EventContext::invalidEventId;
            obs_ = nullptr;
        }

      private:
        EventRegistry::EventIdType id_;
        ObservedMemberType obs_;
    };

    /**
     * @brief This function allows to listen to changes of an observed value by registering an event to it. The returned
     * EventIdType can be used to manually remove the event from the event context, but it is recommended to use
     * smartListen instead, which returns a ListenRemover that automatically removes the event listen.
     *
     * The onEvent function can optionally return a bool, which will be used to determine whether the event should be
     * reinserted into the event context after execution. If onEvent returns false, the event will be removed from the
     * event context.
     *
     * Be careful to NOT cause any more changes to any observed value within the onEvent function, as this will cause
     * infinite recursion. If you need to cause changes to observed values, consider using smartListen, which will delay
     * the execution of onEvent until after all currently active events have been executed, allowing you to safely cause
     * changes to observed values within onEvent. Alternatively use the delayToAfterProcessing function of the event
     * context to delay specific changes until after processing manually.
     *
     * @tparam ValueT The type of the contained observed value.
     * @tparam FunctionT The type of the onEvent function. Needs to be of signature that accepts the observed value
     * contents as an argument.
     * @param obs The observed value to listen to.
     * @param onEvent The function to call when the observed value changes.
     * @return EventRegistry::EventIdType The id of the registered event.
     */
    template <typename ValueT, typename FunctionT>
    EventRegistry::EventIdType listen(Observed<ValueT> const& obs, FunctionT&& onEvent)
    {
        const auto eventId = obs.eventContext().registerEvent(
            Event{
                [obs = Detail::CopyableObservedWrap{obs},
                 onEvent = std::forward<FunctionT>(onEvent)](std::size_t) mutable {
                    if constexpr (std::is_same_v<decltype(onEvent(obs.value())), bool>)
                        return onEvent(obs.value());
                    else
                        onEvent(obs.value());
                    return true;
                },
                []() {
                    return true;
                }});
        obs.attachEvent(eventId);
        return eventId;
    }

    /**
     * @brief This function allows to listen to changes of an observed value by registering an event to it. The returned
     * EventIdType can be used to manually remove the event from the event context, but it is recommended to use
     * smartListen instead, which returns a ListenRemover that automatically removes the event listen.
     *
     * The onEvent function can optionally return a bool, which will be used to determine whether the event should be
     * reinserted into the event context after execution. If onEvent returns false, the event will be removed from the
     * event context.
     *
     * Be careful to NOT cause any more changes to any observed value within the onEvent function, as this will cause
     * infinite recursion. If you need to cause changes to observed values, consider using smartListen, which will delay
     * the execution of onEvent until after all currently active events have been executed, allowing you to safely cause
     * changes to observed values within onEvent. Alternatively use the delayToAfterProcessing function of the event
     * context to delay specific changes until after processing manually.
     *
     * The overload accepting a shared_ptr to an observed value is useful when the observed value can go out of scope
     * while the event is still registered, as it allows the event to be automatically removed when the observed value
     * is destroyed.
     *
     * @tparam ValueT The type of the contained observed value.
     * @tparam FunctionT The type of the onEvent function. Needs to be of signature that accepts the observed value
     * contents as an argument.
     * @param obs A shared_ptr of an observed value to listen to.
     * @param onEvent The function to call when the observed value changes.
     * @return EventRegistry::EventIdType The id of the registered event.
     */
    template <typename ValueT, typename FunctionT>
    EventRegistry::EventIdType listen(std::shared_ptr<Observed<ValueT>> const& obs, FunctionT&& onEvent)
    {
        const auto eventId = obs->eventContext().registerEvent(
            Event{
                [weak = std::weak_ptr<Observed<ValueT>>{obs},
                 onEvent = std::forward<FunctionT>(onEvent)](std::size_t) mutable {
                    if (auto obs = weak.lock(); obs)
                    {
                        if constexpr (std::is_same_v<decltype(onEvent(obs->value())), bool>)
                            return onEvent(obs->value());
                        else
                            onEvent(obs->value());
                        return true;
                    }
                    return false;
                },
                [weak = std::weak_ptr<Observed<ValueT>>{obs}]() {
                    return !weak.expired();
                }});
        obs->attachEvent(eventId);
        return eventId;
    }

    /**
     * @brief Similar to listen but returns a ListenRemover that automatically removes the event on destruction. It also
     * delays the execution of onEvent until after all currently active events have been executed, allowing you to
     * safely cause changes to observed values within onEvent without causing infinite recursion.
     *
     * @tparam ValueT The type of the contained observed value.
     * @tparam FunctionT The type of the onEvent function. Needs to be of signature that accepts the observed value
     * contents as an argument.
     * @param obs The observed value to listen to.
     * @param onEvent The function to call when the observed value changes.
     * @return ListenRemover<Observed<ValueT>> A ListenRemover that will automatically remove the event on destruction.
     */
    template <typename ValueT, typename FunctionT>
    [[nodiscard("The returned ListenRemover must be stored to keep the listener alive")]]
    ListenRemover<Observed<ValueT>> smartListen(Observed<ValueT> const& obs, FunctionT&& onEvent)
    {
        const auto eventId = listen(obs, [fn = std::forward<FunctionT>(onEvent), &obs](auto&& value) mutable {
            obs.eventContext().delayToAfterProcessing([&fn, value = std::forward<decltype(value)>(value)]() mutable {
                fn(std::forward<decltype(value)>(value));
            });
        });
        return ListenRemover{eventId, obs};
    }

    /**
     * @brief Similar to listen but returns a ListenRemover that automatically removes the event on destruction. It also
     * delays the execution of onEvent until after all currently active events have been executed, allowing you to
     * safely cause changes to observed values within onEvent without causing infinite recursion.
     *
     * @tparam ValueT The type of the contained observed value.
     * @tparam FunctionT The type of the onEvent function. Needs to be of signature that accepts the observed value
     * contents as an argument.
     * @param obs A shared pointer to the observed value to listen to.
     * @param onEvent The function to call when the observed value changes.
     * @return ListenRemover<std::shared_ptr<Observed<ValueT>>> A ListenRemover that will automatically remove the event
     * on destruction.
     */
    template <typename ValueT, typename FunctionT>
    [[nodiscard("The returned ListenRemover must be stored to keep the listener alive")]]
    ListenRemover<std::shared_ptr<Observed<ValueT>>>
    smartListen(std::shared_ptr<Observed<ValueT>> const& obs, FunctionT&& onEvent)
    {
        const auto eventId = listen(obs, [fn = std::forward<FunctionT>(onEvent), obs](auto&& value) mutable {
            obs->eventContext().delayToAfterProcessing([fn, value = std::forward<decltype(value)>(value)]() mutable {
                fn(std::forward<decltype(value)>(value));
            });
        });
        return ListenRemover{eventId, obs};
    }
}