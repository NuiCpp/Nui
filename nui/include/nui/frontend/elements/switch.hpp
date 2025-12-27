#pragma once

#include <nui/frontend/elements/fragment.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/utility/overloaded.hpp>
#include <nui/frontend/api/console.hpp>

#include <utility>

namespace Nui::Elements
{
    namespace Detail
    {
        template <typename T>
        struct CaseBaked
        {
            T value;
            Nui::ElementRenderer renderer;
        };

        template <typename T>
        struct Case
        {
            T value;

            auto operator()(Nui::ElementRenderer renderer) &&
            {
                return CaseBaked<T>{std::move(value), std::move(renderer)};
            }
        };

        struct DefaultBaked
        {
            Nui::ElementRenderer renderer;
        };

        struct Default
        {
            auto operator()(Nui::ElementRenderer renderer) &&
            {
                return DefaultBaked{std::move(renderer)};
            }
        };
    }

    template <typename T>
    inline auto case_(T&& value)
    {
        return Detail::Case<T>{std::forward<T>(value)};
    }
    inline auto default_()
    {
        return Detail::Default{};
    }

    template <typename T>
    constexpr auto switch_(Observed<T>& observed)
    {
        return overloaded{
            [&observed](auto&&... bakedCases) {
                return fragment(
                    observe(observed),
                    [&observed, ... cases = std::forward<std::decay_t<decltype(bakedCases)>>(bakedCases)] {
                        Nui::ElementRenderer renderer;
                        return !((cases.value == observed.value() ? (renderer = cases.renderer, true) : false) || ...)
                        ? [](auto&, auto const&) -> std::shared_ptr<Dom::Element> {
                            WebApi::Console::warn("Nui::switch_ error! No case matched and no default case was provided!");
                            return nullptr;
                        } : renderer;
                    });
            },
            [&observed](Detail::DefaultBaked bakedDefault, auto&&... bakedCases) {
                return fragment(
                    observe(observed),
                    [&observed,
                     bakedDefault = std::move(bakedDefault),
                     ... cases = std::forward<std::decay_t<decltype(bakedCases)>>(bakedCases)]() {
                        Nui::ElementRenderer renderer;
                        return !((cases.value == observed.value() ? (renderer = cases.renderer, true) : false) || ...)
                            ? bakedDefault.renderer
                            : renderer;
                    });
            },
        };
    }
}