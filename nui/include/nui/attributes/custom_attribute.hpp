#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

#include <utility>
#include <tuple>
#include <optional>
#include <string>

namespace Nui
{
    template <typename T, typename Tag>
    class CustomAttribute
    {
      public:
        template <typename U = T>
        CustomAttribute(U&& param)
            : param_(std::forward<U>(param))
        {}
        T get() const
        {
            return param_;
        }

      private:
        T param_;
    };

    namespace Detail
    {
        class InvalidAttribute
        {
          public:
            int extract() &&
            {
                return 0;
            }
        };

        template <typename Tag, typename T>
        struct IsAttributeOfTag
        {
            static constexpr bool value = false;
        };
        template <typename Tag, typename T>
        struct IsAttributeOfTag<Tag, CustomAttribute<T, Tag>>
        {
            static constexpr bool value = true;
        };

        template <typename Tag, typename... Attributes>
        constexpr static auto* extractAttributeImpl()
        {
            return static_cast<InvalidAttribute*>(nullptr);
        }

        template <typename Tag, typename Front, typename... Attributes>
        constexpr static auto* extractAttributeImpl(Front& front, Attributes&... attributes)
        {
            if constexpr (IsAttributeOfTag<Tag, Front>::value)
            {
                return &front;
            }
            else
            {
                return extractAttributeImpl<Tag>(attributes...);
            }
        }
    }

    template <typename Tag, typename... Attributes>
    constexpr auto* extractOptionalAttribute(Attributes&... attributes)
    {
        return Detail::extractAttributeImpl<Tag>(attributes...);
    }

    template <typename Tag, typename... Attributes>
    constexpr auto extractAttribute(Attributes&... attributes)
    {
        auto* const attr = extractOptionalAttribute<Tag>(attributes...);
        // If you land here, a component expects an attribute to be passed, but didnt.
        static_assert(
            !std::is_same_v<Detail::InvalidAttribute* const, decltype(attr)>,
            "Attribute is required. Look for a 'extractAttribute<TAG_HERE>' in the error log");
        return attr->get();
    }

#define NUI_MAKE_CUSTOM_ATTRIBUTE(NAME) \
    struct NAME##Tag \
    { \
        template <typename T> \
        std::enable_if_t<!::Nui::Detail::IsObserved_v<std::decay_t<T>>, CustomAttribute<T, NAME##Tag>> \
        operator=(T&& param) const \
        { \
            return std::forward<T>(param); \
        } \
        template <typename T> \
        std::enable_if_t<::Nui::Detail::IsObserved_v<std::decay_t<T>>, CustomAttribute<T&, NAME##Tag>> \
        operator=(T& param) const \
        { \
            return {param}; \
        } \
        template <typename GeneratorType, typename... ObservedValues> \
        CustomAttribute<ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...>, NAME##Tag> \
        operator=(ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...> const& combinator) const \
        { \
            return CustomAttribute<ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...>, NAME##Tag>{ \
                combinator}; \
        } \
    } static constexpr NAME;
}