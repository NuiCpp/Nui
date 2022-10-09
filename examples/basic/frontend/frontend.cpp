#include <nui/core.hpp>
#include <nui/window.hpp>

#include <emscripten/val.h>
#include <nui/dom/dom.hpp>
#include <nui/dom/element.hpp>
#include <nui/elements.hpp>
#include <nui/attributes.hpp>
#include <nui/observed.hpp>

using namespace Nui;

void frontendMain()
{
    namespace attr = Nui::Attributes;
    using attr::id;
    using attr::onMouseLeave;
    using attr::onMouseEnter;

    thread_local Dom::Dom dom;
    thread_local Observed<std::string> style = "background-color: #ff00000;";
    thread_local Observed<bool> condition = false;

    using Nui::div; // there is a global symbol named div
    using Dom::Element;

    // clang-format off
    const auto body = div{
        attr::style = style,
    }(
        div{
            id = "hi"
        }(
            div{
                id = "deep"
            }(
                observe(condition),
                []() -> std::function<std::shared_ptr<Element>(Element&)>{
                    if (!*condition)
                    {
                        return div{
                            id = "no",
                            attr::style = "background-color: red; width: 100px; height: 100px;",
                            onMouseEnter = [](emscripten::val event) {
                                condition = true;
                            }
                        }();
                    }
                    else {
                        return div{
                            id = "yes",
                            attr::style = "background-color: green; width: 100px; height: 100px;",
                            onMouseLeave = [](emscripten::val event) {
                                condition = false;
                            }
                        }();
                    }
                }
            )
        )
    );
    //  clang-format on

    dom.root().appendElement(body);

    //style = "background-color: black; height: 100px; width: 100px;";
}

EMSCRIPTEN_BINDINGS(mymod)
{
    emscripten::function("main", &frontendMain);
}

#include <nui/bindings.hpp>