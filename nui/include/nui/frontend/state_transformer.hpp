#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/frontend/api/console.hpp>

#include <utility>
#include <any>
#include <functional>
#include <tuple>
#include <type_traits>
#include <memory>

namespace Nui
{
    /**
     * @brief An enum to decide if an assignment shall be done directly or through observed.value(). So it either causes
     * events to fire or not.
     */
    enum class ChangePolicy
    {
        Tracked,
        Untracked
    };

    /**
     * @brief Useful base class to refer to a StateTransformer without the need to know the reification strategies.
     */
    class StateTransformerBase
    {
      public:
        virtual ~StateTransformerBase() = default;
        StateTransformerBase() = default;
        StateTransformerBase(StateTransformerBase const&) = default;
        StateTransformerBase& operator=(StateTransformerBase const&) = default;
        StateTransformerBase(StateTransformerBase&&) = default;
        StateTransformerBase& operator=(StateTransformerBase&&) = default;
    };

    /**
     * @brief A class that transforms state using the provided reification strategies. It can be constructed from a
     * value or an observed value or an observed value combinator. The reification strategies are used to transform the
     * state into a form that can be used for rendering. The assigner is used to assign a new value to the state, either
     * through the observed value or directly. The value can be retrieved using the value() method, which will return
     * the current value of the state.
     *
     * Make sure that when you retrieve the value using value(), you use the correct type. The type is determined by the
     * constructor used to create the StateTransformer. If you created it from a value of type T, then you should
     * retrieve it as T. If you created it from an observed value of type T, you should also retrieve it as T.
     *
     * @tparam ReificationStrategies The strategies used to reify the state for rendering. Each strategy should have a
     * static reify method that takes the state and returns an ElementRenderer or an Attribute.
     */
    template <typename... ReificationStrategies>
    class StateTransformer : public StateTransformerBase
    {
      public:
        ~StateTransformer() override = default;
        StateTransformer() = delete;
        StateTransformer(StateTransformer const&) = default;
        StateTransformer& operator=(StateTransformer const&) = default;
        StateTransformer(StateTransformer&&) = default;
        StateTransformer& operator=(StateTransformer&&) = default;

      private:
        template <typename U>
        requires(!Nui::IsObservedLike<std::decay_t<U>>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(std::shared_ptr<U> value)
            : reifier_{[this, value]() mutable {
                return std::make_tuple(ReificationStrategies::reify(*this, *value)...);
            }}
            , assigner_{[value](void const* ptr, ChangePolicy) {
                *value = *static_cast<U const*>(ptr);
            }}
            , value_{[value]() mutable {
                return *value;
            }}
        {}

      public:
        /**
         * @brief Constructor for a plain value. The value is stored internally and can be retrieved using value().
         *
         * @tparam U
         */
        template <typename U>
        requires(!Nui::IsObservedLike<std::decay_t<U>>)
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(U value)
            : StateTransformer(std::make_shared<U>(std::move(value)))
        {}

        /**
         * @brief Constructor for a shared_ptr to an observed value.
         *
         * @tparam U Observed state type
         * @tparam Tags Observed state tags
         */
        template <typename U, typename Tags>
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(std::shared_ptr<Nui::Observed<U, Tags>> const& value)
            : reifier_{[this, value]() mutable {
                return std::make_tuple(ReificationStrategies::reify(*this, *value)...);
            }}
            , assigner_{[value](void const* ptr, ChangePolicy status) {
                if (status == ChangePolicy::Tracked)
                    *value = *static_cast<U const*>(ptr);
                else
                    value->value() = *static_cast<U const*>(ptr);
            }}
            , value_{[value]() mutable {
                return value->value();
            }}
        {}

        /**
         * @brief Constructor for a reference to an observed value. Make sure that the observed value outlives the
         * StateTransformer, otherwise accessing the value or reifying will lead to undefined behavior.
         *
         * @tparam U Observed state type
         * @tparam Tags Observed state tags
         */
        template <typename U, typename Tags>
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(Nui::Observed<U, Tags>& val)
            : reifier_{[this, ref = std::reference_wrapper<Nui::Observed<U, Tags>>{val}]() mutable {
                return std::make_tuple(ReificationStrategies::reify(*this, ref.get())...);
            }}
            , assigner_{[ref = std::reference_wrapper<Nui::Observed<U, Tags>>{val}](
                            void const* ptr,
                            ChangePolicy status) {
                if (status == ChangePolicy::Tracked)
                    ref.get() = *static_cast<U const*>(ptr);
                else
                    ref.get().value() = *static_cast<U const*>(ptr);
            }}
            , value_{[ref = std::reference_wrapper<Nui::Observed<U, Tags>>{val}]() mutable {
                return ref.get().value();
            }}
        {}

        /**
         * @brief Constructor for a weak_ptr to an observed value. The weak_ptr is locked internally to access the
         * value. Make sure that the observed value outlives the StateTransformer, otherwise the functions will throw.
         *
         * @tparam U Observed state type
         * @tparam Tags Observed state tags
         */
        template <typename U, typename Tags>
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(std::weak_ptr<Nui::Observed<U, Tags>> const& val)
            : reifier_{[this, val = std::weak_ptr{val.lock()}]() mutable {
                auto value = val.lock();
                if (!value)
                    throw std::runtime_error(
                        "StateTransformer: Failed to lock weak_ptr to observed value. The observed value may have been "
                        "destroyed.");

                return std::make_tuple(ReificationStrategies::reify(*this, *value)...);
            }}
            , assigner_{[val = std::weak_ptr{val.lock()}](void const* ptr, ChangePolicy status) {
                auto value = val.lock();
                if (!value)
                    throw std::runtime_error(
                        "StateTransformer: Failed to lock weak_ptr to observed value. The observed value may have been "
                        "destroyed.");

                if (status == ChangePolicy::Tracked)
                    *value = *static_cast<U const*>(ptr);
                else
                    value->value() = *static_cast<U const*>(ptr);
            }}
            , value_{[val = std::weak_ptr{val.lock()}]() mutable {
                auto value = val.lock();
                if (!value)
                    throw std::runtime_error(
                        "StateTransformer: Failed to lock weak_ptr to observed value. The observed value may have been "
                        "destroyed.");

                return value->value();
            }}
        {}

      private:
        template <typename RendererType, typename... ObservedValues>
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(
            std::shared_ptr<Nui::ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...>>
                sharedCombinator)
            : reifier_{[this, sharedCombinator]() mutable {
                return std::make_tuple(ReificationStrategies::reify(*this, *sharedCombinator)...);
            }}
            , assigner_{}
            , value_{[sharedCombinator]() mutable {
                return sharedCombinator->value();
            }}
        {}

      public:
        /**
         * @brief Constructor for a StateTransformer with a combinator. Make sure that referenced observed values
         * outlive the StateTransformer, otherwise accessing the value or reifying will lead to undefined behavior.
         *
         * @tparam RendererType The type of the generator function in the combinator.
         * @tparam ObservedValues The types of the observed values in the combinator.
         */
        template <typename RendererType, typename... ObservedValues>
        // NOLINTNEXTLINE(hicpp-explicit-conversions): Intentional
        StateTransformer(Nui::ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...> combinator)
            : StateTransformer(
                  std::make_shared<Nui::ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...>>(
                      std::move(combinator)))
        {}

        /**
         * @brief Assigns a value to the state if it can be assigned (Observed<T>, shared_ptr<Observed<T>>,
         * weak_ptr<Observed<T>>).
         *
         * @tparam T The type of the value to assign.
         * @param value The value to assign.
         * @param status The change policy to use when assigning the value.
         */
        template <typename T>
        void assign(T const& value, ChangePolicy status = ChangePolicy::Tracked) const
        {
            if (assigner_)
                assigner_(&value, status);
        }

        /**
         * @brief Accesses the current value of the state. The type T should match the type used in the constructor. If
         * the stored type does not match the requested type, a std::bad_any_cast exception will be thrown.
         *
         * @tparam T The type of the value to retrieve.
         * @return T A copy of the current value of the state.
         */
        template <typename T>
        T value() const
        {
            try
            {
                return std::any_cast<T>(value_());
            }
            catch (const std::bad_any_cast& e)
            {
                Nui::WebApi::Console::error(
                    "StateTransformer: Failed to retrieve value. The stored type does not match the requested type.");
                throw e;
            }
        }

        /**
         * @brief Reifies the current state into a tuple of values using the specified reification strategies.
         *
         * @return std::tuple<typename ReificationStrategies::type...>
         */
        std::tuple<typename ReificationStrategies::type...> reify() const
        {
            return reifier_();
        }

      private:
        std::function<std::tuple<typename ReificationStrategies::type...>()> reifier_;
        std::function<void(void const*, ChangePolicy)> assigner_;
        std::function<std::any()> value_;
    };
}