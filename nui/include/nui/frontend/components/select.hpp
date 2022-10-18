#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/attributes/impl/custom_attribute.hpp>
#include <nui/frontend/components/model/select.hpp>
#include <nui/frontend/dom/reference.hpp>

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
        NUI_MAKE_CUSTOM_ATTRIBUTE(selectReference);
    }

    template <
        template <typename...>
        typename ContainerT,
        typename ValueType,
        typename OnChange,
        typename ReferencePasserT,
        typename... SelectAttributes>
    requires(!IsCustomAttribute<SelectAttributes> && ...) constexpr auto SelectImpl(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<int, preSelectTag>&& preSelectedIndex,
        CustomAttribute<OnChange, onSelectTag>&& onSelect,
        CustomAttribute<ReferencePasserT, selectReferenceTag>&& selectReference,
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
            Dom::reference(selectReference.get()),
            selectModel.get().map([preSelectedIndex = preSelectedIndex.get()](auto i, auto const& opt) {
                return option{
                    value = opt.value,
                    selected = i == preSelectedIndex,
                }(opt.label);
            })
        );
        // clang-format on
    }

    template <
        template <typename...>
        typename ContainerT,
        typename ValueType,
        typename OnChange,
        typename ReferencePasserT,
        typename... SelectAttributes>
    requires(!IsCustomAttribute<SelectAttributes> && ...) constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<int, preSelectTag>&& preSelectedIndex,
        CustomAttribute<OnChange, onSelectTag>&& onSelect,
        CustomAttribute<ReferencePasserT, selectReferenceTag>&& selectReference,
        SelectAttributes&&... selectAttributes)
    {
        return SelectImpl(
            std::move(selectModel),
            std::move(preSelectedIndex),
            std::move(onSelect),
            std::move(selectReference),
            std::forward<SelectAttributes>(selectAttributes)...);
    }

    template <template <typename...> typename ContainerT, typename ValueType, typename... SelectAttributes>
    requires(!IsCustomAttribute<SelectAttributes> && ...) constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<int, preSelectTag>&& preSelectedIndex,
        SelectAttributes&&... selectAttributes)
    {
        using namespace SelectAttributes;
        return SelectImpl(
            std::move(selectModel),
            std::move(preSelectedIndex),
            onSelect = [](int) {},
            selectReference = [](std::weak_ptr<Dom::BasicElement>&&) {},
            std::forward<SelectAttributes>(selectAttributes)...);
    }

    template <
        template <typename...>
        typename ContainerT,
        typename OnChange,
        typename ValueType,
        typename... SelectAttributes>
    requires(!IsCustomAttribute<SelectAttributes> && ...) constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        CustomAttribute<OnChange, onSelectTag>&& onSelect,
        SelectAttributes&&... selectAttributes)
    {
        using namespace SelectAttributes;
        return SelectImpl(
            std::move(selectModel),
            preSelect = -1,
            std::move(onSelect),
            selectReference = [](std::weak_ptr<Dom::BasicElement>&&) {},
            std::forward<SelectAttributes>(selectAttributes)...);
    }

    template <template <typename...> typename ContainerT, typename ValueType, typename... SelectAttributes>
    requires(!IsCustomAttribute<SelectAttributes> && ...) constexpr auto Select(
        CustomAttribute<Observed<ContainerT<SelectOptions<ValueType>>>&, selectModelTag>&& selectModel,
        SelectAttributes&&... selectAttributes)
    {
        using namespace SelectAttributes;
        return SelectImpl(
            std::move(selectModel),
            preSelect = -1,
            onSelect = [](int) {},
            selectReference = [](std::weak_ptr<Dom::BasicElement>&&) {},
            std::forward<SelectAttributes>(selectAttributes)...);
    }
}