#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/components/model/select.hpp>

// Elements
#include <nui/frontend/elements/option.hpp>
#include <nui/frontend/elements/select.hpp>

// Attributes
#include <nui/frontend/attributes/selected.hpp>
#include <nui/frontend/attributes/value.hpp>
#include <nui/frontend/attributes/on_change.hpp>

#include <vector>
#include <functional>

namespace Nui::Components
{
    template <template <typename...> typename ContainerT, typename ValueT>
    struct SelectArgs
    {
        Observed<ContainerT<SelectOptions<ValueT>>>& model;
        int preSelectedIndex = -1;
        std::function<void(long long, SelectOptions<ValueT> const&)> onSelect = {};
        std::vector<Attribute> selectAttributes = {};
    };

    template <typename ValueT, template <typename...> typename ContainerT = std::vector>
    constexpr auto Select(SelectArgs<ContainerT, ValueT>&& args)
    {
        using namespace Attributes;
        using namespace Elements;

        auto attributes = std::move(args.selectAttributes);
        if (args.onSelect)
            attributes.push_back(onChange = [onSelect = std::move(args.onSelect), &model = args.model](Nui::val event) {
                const auto index = event["target"]["selectedIndex"].as<long long>();
                onSelect(index, model.value()[index]);
            });

        // clang-format off
        return select{
            std::move(attributes)
        }(
            args.model.map([preSelectedIndex = args.preSelectedIndex](auto i, auto const& opt) {
                return option{
                    value = opt.value,
                    selected = (i == preSelectedIndex),
                }(opt.label);
            })
        );
        // clang-format on
    }
}