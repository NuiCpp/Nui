#pragma once

#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/utility/meta/tuple_filter.hpp>
#include <nui/utility/meta/tuple_transform.hpp>
#include <nui/utility/meta/pick_first.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <mplex/control/if.hpp>
#include <mplex/functional/lift.hpp>
#include <mplex/fundamental/integral.hpp>

#include <tuple>
#include <string>
#include <sstream>
#include <optional>

namespace Nui::Attributes
{
    namespace Detail
    {
        template <typename... T>
        struct StylePropertyEbo
        {
            std::tuple<Observed<T> const&...> observed;
        };
        template <>
        struct StylePropertyEbo<void>
        {};
    }
    template <typename FunctionT, typename... T>
    struct StylePropertyImpl : public Detail::StylePropertyEbo<T...>
    {
        using value_type = std::tuple<Observed<T> const&...>;

        FunctionT generator;
        constexpr static bool isStatic()
        {
            return sizeof...(T) == 1 && std::is_same_v<Nui::Detail::PickFirst_t<T...>, void>;
        }
        constexpr auto operator()() const
        {
            return generator();
        }

        template <typename U>
        constexpr StylePropertyImpl(FunctionT generator, Observed<U> const& observed)
            : Detail::StylePropertyEbo<T...>{std::forward_as_tuple(observed)}
            , generator{std::move(generator)}
        {}
        constexpr StylePropertyImpl(FunctionT generator, std::nullptr_t)
            : Detail::StylePropertyEbo<T...>{}
            , generator{std::move(generator)}
        {}
        template <typename GeneratorT>
        constexpr StylePropertyImpl(
            FunctionT generator,
            ObservedValueCombinatorWithGenerator<GeneratorT, Observed<T>...>&& observed)
            : Detail::StylePropertyEbo<T...>{std::move(observed).observedValues()}
            , generator{std::move(generator)}
        {}
    };
    template <typename FunctionT>
    StylePropertyImpl(FunctionT generator, std::nullptr_t) -> StylePropertyImpl<FunctionT, void>;
    template <typename FunctionT, typename T>
    StylePropertyImpl(FunctionT generator, Observed<T>&) -> StylePropertyImpl<FunctionT, T>;
    template <typename FunctionT, typename GeneratorT, typename... ObservedT>
    StylePropertyImpl(FunctionT generator, ObservedValueCombinatorWithGenerator<GeneratorT, ObservedT...>&&)
        -> StylePropertyImpl<FunctionT, typename ObservedT::value_type...>;

    namespace Detail
    {
        template <typename T>
        struct IsDynamicStyleProperty
        {
            constexpr static bool value = true;
        };
        template <typename FunctionT>
        struct IsDynamicStyleProperty<StylePropertyImpl<FunctionT, void>>
        {
            constexpr static bool value = false;
        };
        template <typename Property>
        struct StripPropertyObserved
        {
            using type = typename Property::value_type;
            constexpr static auto extract(Property& prop)
            {
                return prop.observed;
            }
        };
        template <typename FunctionT>
        struct StripPropertyObserved<StylePropertyImpl<FunctionT, void>>
        {
            using type = std::tuple<>;
            constexpr static void* extract(StylePropertyImpl<FunctionT, void>&)
            {
                return nullptr;
            }
        };

        template <typename IntegerSequence, typename CurrentIndex, typename... Properties>
        struct BuildObservedPropertyIndexList;

        template <unsigned... Indices, typename CurrentIndex, typename Property, typename... Properties>
        struct BuildObservedPropertyIndexList<
            std::integer_sequence<unsigned, Indices...>,
            CurrentIndex,
            Property,
            Properties...>
        {
            // TODO: eliminate lift, by transforming this class to an mplex functor concept.
            using type = mplex::lazy_if_vt<
                IsDynamicStyleProperty<Property>::value,
                mplex::then_<
                    mplex::lift<BuildObservedPropertyIndexList>,
                    std::integer_sequence<unsigned, Indices..., CurrentIndex::value>,
                    mplex::unsigned_<CurrentIndex::value + 1u>,
                    Properties...>,
                mplex::else_<
                    mplex::lift<BuildObservedPropertyIndexList>,
                    std::integer_sequence<unsigned, Indices...>,
                    mplex::unsigned_<CurrentIndex::value + 1u>,
                    Properties...>>;
        };

        template <unsigned... Indices, typename CurrentIndex>
        struct BuildObservedPropertyIndexList<std::integer_sequence<unsigned, Indices...>, CurrentIndex>
        {
            using type = std::integer_sequence<unsigned, Indices...>;
        };

        template <typename IndexList>
        struct ExtractObservedValuesFromProperties;
        template <unsigned... Indices>
        struct ExtractObservedValuesFromProperties<std::integer_sequence<unsigned, Indices...>>
        {
            constexpr static auto apply(auto&& propertyTuple)
            {
                return std::tuple_cat(std::get<Indices>(propertyTuple).observed...);
            }
        };
    }

    struct StyleProperty
    {
        char const* name;
        constexpr StyleProperty(char const* name)
            : name{name}
        {}
        // TODO: optimize following functions:
        auto operator=(char const* value)
        {
            return StylePropertyImpl{
                [name = std::string{name}, value = std::string{value}]() {
                    return name + ":" + value;
                },
                nullptr};
        }
        auto operator=(std::string value)
        {
            return StylePropertyImpl{
                [name = std::string{name}, value = std::move(value)]() {
                    return name + ":" + value;
                },
                nullptr};
        }
        auto operator=(Observed<std::string>& observedValue)
        {
            return StylePropertyImpl{
                [name = std::string{name}, &observedValue]() {
                    return name + ":" + observedValue.value();
                },
                observedValue};
        }
        template <typename FunctionT, typename... ArgsT>
        auto operator=(ObservedValueCombinatorWithGenerator<FunctionT, ArgsT...>&& combinator)
        {
            return StylePropertyImpl{
                [name = std::string{name}, gen = combinator.generator()]() {
                    return name + ":" + gen();
                },
                std::move(combinator)};
        }
    };

    inline namespace Literals
    {
        static constexpr StyleProperty operator"" _style(char const* name, std::size_t)
        {
            return StyleProperty{name};
        };
    }

    namespace Detail
    {
        template <bool isStatic, typename... Properties>
        auto makeStyleGenerator(Properties&&... props)
        {
            return [... props = std::forward<Properties>(props)]() {
                // TODO: better performing version:
                std::stringstream sstr;
                [&sstr](auto const& head, auto const&... tail) {
                    using expander = int[];
                    sstr << head();
                    (void)expander{0, (sstr << ";" << tail(), void(), 0)...};
                }(props...);
                return sstr.str();
            };
        }
    }

    namespace Detail
    {
        template <typename... Properties>
        constexpr auto stripObserved(Properties&... props)
        {
            return ExtractObservedValuesFromProperties<typename BuildObservedPropertyIndexList<
                std::integer_sequence<unsigned>,
                mplex::unsigned_<0>,
                Properties...>::type>::apply(std::tie(props...));
        }
    }

    template <typename... Properties>
    class Style
    {
      public:
        using ObservedValueList = Nui::Detail::FlatTupleTransform_t<
            Nui::Detail::TupleFilter_t<Detail::IsDynamicStyleProperty, Properties...>,
            Detail::StripPropertyObserved>;

        constexpr Style(Properties&&... props)
            : observedValues_{stripObserved(props...)}
            , generateStyle_{makeStyleGenerator<isStatic()>(std::forward<Properties>(props)...)}
        {}

        constexpr static bool isStatic()
        {
            return (Properties::isStatic() && ...);
        }

        std::string toString() const
        {
            return generateStyle_();
        }

        std::function<std::string()> ejectGenerator() &&
        {
            return std::move(generateStyle_);
        }

        ObservedValueList ejectObservedValues() &&
        {
            return std::move(observedValues_);
        }

      private:
        ObservedValueList observedValues_;
        std::function<std::string()> generateStyle_;
    };

    struct style_
    {
        template <typename U>
        requires(!IsObserved<std::decay_t<U>>)
        constexpr Attribute<std::decay_t<U>> operator=(U val) const
        {
            return Attribute<std::decay_t<U>>{std::move(val)};
        }
        template <typename U>
        requires(IsObserved<std::decay_t<U>>)
        constexpr Attribute<std::decay_t<U>> operator=(U& val) const
        {
            return Attribute<std::decay_t<U>>{val};
        }
        template <typename... T>
        constexpr auto operator=(Style<T...>&& style) const
        {
            if constexpr (Style<T...>::isStatic())
            {
                return Attribute<std::string>{style.toString()};
            }
            else
            {
                return std::apply(
                    [&style]<typename... ObservedValueTypes>(ObservedValueTypes&... obs) {
                        return Attribute<
                            ObservedValueCombinatorWithGenerator<std::function<std::string()>, ObservedValueTypes...>>{
                            ObservedValueCombinator<ObservedValueTypes...>{obs...}.generate(
                                std::move(style).ejectGenerator())};
                    },
                    std::move(style).ejectObservedValues());
            }
        }
    } static constexpr style;
}