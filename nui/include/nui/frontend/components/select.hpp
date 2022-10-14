#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/attributes/impl/custom_attribute.hpp>
#include <nui/frontend/components/model/select.hpp>

// Elements
#include <nui/frontend/elements/option.hpp>
#include <nui/frontend/elements/select.hpp>

// Attributes
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/selected.hpp>
#include <nui/frontend/attributes/type.hpp>
#include <nui/frontend/attributes/value.hpp>
#include <nui/frontend/attributes/on_change.hpp>

#include <string>
#include <iterator>
#include <iostream>

namespace Nui::Components
{
    inline namespace SelectAttributes
    {
        NUI_MAKE_CUSTOM_ATTRIBUTE(selectModel);
        NUI_MAKE_CUSTOM_ATTRIBUTE(preSelect);
        NUI_MAKE_CUSTOM_ATTRIBUTE(onSelect);
    }

    template <
        template <typename...>
        typename ContainerT,
        typename ValueType,
        typename OnChange,
        typename... SelectAttributes>
    constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<int, preSelectTag>&& preSelectedIndex,
        CustomAttribute<OnChange, onSelectTag>&& onSelect,
        SelectAttributes&&... selectAttributes)
    {
        using namespace Attributes;
        using namespace Elements;

        // clang-format off
        return select{
            onChange = [onSelect = onSelect.get()](emscripten::val event){
                onSelect(event["target"]["selectedIndex"].as<int>());
            },
            std::forward<SelectAttributes>(selectAttributes)...,
        }(
            selectModel.get().map([preSelectedIndex = preSelectedIndex.get()](auto i, auto const& opt) {
                return option{
                    value = opt.value,
                    selected = i == preSelectedIndex,
                }(opt.label);
            })
        );
        // clang-format on
    }

    template <template <typename...> typename ContainerT, typename ValueType, typename... SelectAttributes>
    constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<int, preSelectTag>&& preSelectedIndex,
        SelectAttributes&&... selectAttributes)
    {
        using namespace SelectAttributes;
        return Select(
            std::move(selectModel),
            std::move(preSelectedIndex),
            onSelect = [](int) {},
            std::forward<SelectAttributes>(selectAttributes)...);
    }

    template <
        template <typename...>
        typename ContainerT,
        typename OnChange,
        typename ValueType,
        typename... SelectAttributes>
    constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<OnChange, onSelectTag>&& onSelect,
        SelectAttributes&&... selectAttributes)
    {
        using namespace SelectAttributes;
        return Select(
            std::move(selectModel),
            preSelect = -1,
            std::move(onSelect),
            std::forward<SelectAttributes>(selectAttributes)...);
    }

    template <template <typename...> typename ContainerT, typename ValueType, typename... SelectAttributes>
    constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        SelectAttributes&&... selectAttributes)
    {
        using namespace SelectAttributes;
        return Select(
            std::move(selectModel),
            preSelect = -1,
            onSelect = [](int) {},
            std::forward<SelectAttributes>(selectAttributes)...);
    }
}