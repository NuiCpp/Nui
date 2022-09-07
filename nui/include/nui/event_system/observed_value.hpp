#pragma once

#include <nui/concepts.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/utility/meta/pick_first.hpp>

#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <list>
#include <utility>

namespace Nui
{
    template <typename ContainedT>
    class Observed
    {
      public:
        class ModificationProxy
        {
          public:
            explicit ModificationProxy(Observed& observed)
                : observed_{observed}
            {}
            ~ModificationProxy()
            {
                try
                {
                    observed_.update();
                }
                catch (...)
                {
                    // TODO: log?
                }
            }
            auto& data()
            {
                return observed_.contained_;
            }
            auto* operator->()
            {
                return &observed_.contained_;
            }

          private:
            Observed& observed_;
        };

      public:
        Observed() = default;
        Observed(const Observed&) = delete;
        Observed(Observed&&) = default;
        Observed& operator=(const Observed&) = delete;
        Observed& operator=(Observed&&) = default;
        ~Observed() = default;

        template <typename T = ContainedT>
        Observed(T&& t)
            : contained_{std::forward<T>(t)}
            , attachedEvents_{}
        {}

        /**
         * @brief Assign a completely new value.
         *
         * @param t
         * @return Observed&
         */
        template <typename T = ContainedT>
        Observed& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            update();
            return *this;
        }

        template <typename T>
        requires Incrementable<T>
        friend Observed<T>& operator++(Observed<T>& observedValue);

        template <typename T>
        requires Incrementable<T>
        friend T operator++(Observed<T>& observedValue, int);

        template <typename T>
        requires Decrementable<T>
        friend Observed<T>& operator--(Observed<T>& observedValue);

        template <typename T>
        requires Decrementable<T>
        friend T operator--(Observed<T>& observedValue, int);

        template <typename T = ContainedT, typename U>
        requires PlusAssignable<T, U> Observed<T>
        &operator+=(U const& rhs)
        {
            this->contained_ += rhs;
            return *this;
        }
        template <typename T = ContainedT, typename U>
        requires MinusAssignable<T, U> Observed<T>
        &operator-=(U const& rhs)
        {
            this->contained_ -= rhs;
            return *this;
        }

        template <typename T = ContainedT>
        requires std::equality_comparable<T> && Fundamental<T> Observed& operator=(T&& t)
        {
            return assignChecked(t);
        }

        template <typename T = ContainedT>
        requires std::equality_comparable<T> Observed& assignChecked(T&& other)
        {
            if (contained_ != other)
            {
                contained_ = std::forward<T>(other);
                update();
            }
            return *this;
        }

        /**
         * @brief Can be used to make mutations to the underlying class that get commited when the returned proxy is
         * destroyed.
         *
         * @return ModificationProxy
         */
        ModificationProxy modify()
        {
            return ModificationProxy{*this};
        }

        void attachEvent(EventContext::EventIdType eventId)
        {
            attachedEvents_.emplace_back(eventId);
        }
        void attachOneshotEvent(EventContext::EventIdType eventId)
        {
            attachedOneshotEvents_.emplace_back(eventId);
        }
        void unattachEvent(EventContext::EventIdType eventId)
        {
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), eventId),
                std::end(attachedEvents_));
        }

        ContainedT& value()
        {
            return contained_;
        }
        ContainedT const& value() const
        {
            return contained_;
        }
        ContainedT& operator*()
        {
            return contained_;
        }
        ContainedT const& operator*() const
        {
            return contained_;
        }

      private:
        void update()
        {
            for (auto& event : attachedEvents_)
            {
                if (!globalEventContext.activateEvent(event))
                    event = EventRegistry::invalidEventId;
            }
            for (auto& event : attachedOneshotEvents_)
                globalEventContext.activateEvent(event);
            attachedOneshotEvents_.clear();
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), EventRegistry::invalidEventId),
                std::end(attachedEvents_));
        }

      private:
        ContainedT contained_;
        std::vector<EventContext::EventIdType> attachedEvents_;
        std::vector<EventContext::EventIdType> attachedOneshotEvents_;
    };
    template <typename T>
    requires Incrementable<T>
    inline Observed<T>& operator++(Observed<T>& observedValue)
    {
        ++observedValue.contained_;
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    requires Incrementable<T>
    inline T operator++(Observed<T>& observedValue, int)
    {
        auto tmp = observedValue.contained_;
        ++observedValue.contained_;
        observedValue.update();
        return tmp;
    }

    template <typename T>
    inline auto operator--(Observed<T>& observedValue)
        -> Observed<Detail::PickFirst_t<T, decltype(--std::declval<T>())>>&
    {
        --observedValue.contained_;
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    inline auto operator--(Observed<T>& observedValue, int) -> Detail::PickFirst_t<T, decltype(std::declval<T>()--)>
    {
        auto tmp = observedValue.contained_;
        --observedValue.contained_;
        observedValue.update();
        return tmp;
    }

    namespace Detail
    {
        template <typename T>
        struct IsObserved
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct IsObserved<Observed<T>>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        constexpr bool IsObserved_v = IsObserved<T>::value;
    }
}