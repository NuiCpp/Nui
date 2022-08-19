#include <nui/core.hpp>
#include <nui/window.hpp>

// FIXME: eradicate ifdef
#ifdef NUI_BACKEND
#    include <nui_basic.hpp>
#else
#    include <emscripten/val.h>
#    include <nui/dom/dom.hpp>
#    include <nui/elements.hpp>
#    include <nui/attributes.hpp>
#    include <nui/observed.hpp>
#endif

#include <iostream>
#include <string_view>

using namespace Nui;

int EMSCRIPTEN_KEEPALIVE main()
{
    Window window{"Basic Example"};
    window.setSize(480, 320, Nui::WebViewHint::WEBVIEW_HINT_NONE);

// FIXME: eradicate ifdef
#ifdef NUI_BACKEND
    window.loadFrontend(nui_basic());
#else
    namespace attr = Nui::Attributes;
    using attr::id;

    thread_local Dom::Dom dom;
    thread_local Observed<std::string> style = "background-color: #ff00000;";

    using Nui::div; // there is a global symbol named div

    std::cout << *style << '\n';

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
                div{
                    id = "deeper"
                }
            )
        )
    );

    dom.root().appendElement(body);
    //  clang-format on

    style = "background-color: cyan; height: 100px; width: 100px;";
#endif
    window.run();

    return 0;
}

// FIXME: eradicate ifdef
#ifdef NUI_FRONTEND
EMSCRIPTEN_BINDINGS(mymod)
{
    emscripten::function("main", &main);
}
#endif