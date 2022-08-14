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
    thread_local Dom::Dom dom;

    using Nui::div; // there is a global symbol named div

    // clang-format off
    const auto body = div{
        style = "background-color: #ff0000;"
    }(
        div{
            id = "hi"
        }
    );

    dom.root().appendElement(body);
    //  clang-format on
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