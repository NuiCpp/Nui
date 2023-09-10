#pragma once

#include <nui/frontend/dom/childless_element.hpp>
#include <nui/frontend/elements/impl/html_element.hpp>

namespace Nui
{
    constexpr auto RegularHtmlElementBridge = HtmlElementBridge{
        .createElement =
            +[](HtmlElement const& element) {
                return Nui::val::global("document").call<Nui::val>("createElement", Nui::val{element.name()});
            },
    };

    constexpr auto SvgElementBridge = HtmlElementBridge{
        .createElement =
            +[](HtmlElement const& element) {
                return Nui::val::global("document")
                    .call<Nui::val>(
                        "createElementNS",
                        Nui::val{std::string{"http://www.w3.org/2000/svg"}},
                        Nui::val{element.name()});
            },
    };

    constexpr auto TextElementBridge = HtmlElementBridge{
        .createElement =
            +[](HtmlElement const& element) {
                return Nui::val::global("document")
                    .call<Nui::val>("createTextNode", Nui::val{element.attributes()[0].stringData()});
            },
    };
}