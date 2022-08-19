#pragma once

#include <nui/concepts.hpp>

#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <list>

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
            , sideEffects_{}
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

        void emplaceSideEffect(std::invocable<ContainedT const&> auto&& sideEffect)
        {
            sideEffects_.emplace_back(std::forward<std::decay_t<decltype(sideEffect)>>(sideEffect));
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
            for (auto effect = std::begin(sideEffects_); effect != std::end(sideEffects_);)
            {
                if (!effect(contained_))
                    effect = sideEffects_.erase(effect);
                else
                    ++effect;
            }
        }

      private:
        ContainedT contained_;
        // TODO: performance container:
        std::list<std::function<bool(ContainedT const&)>> sideEffects_;
    };

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