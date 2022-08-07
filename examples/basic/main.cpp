#include <nui/core.hpp>
#include <nui/window.hpp>
#include <nui/dom.hpp>
#include <nui/elements.hpp>
#include <nui/attributes.hpp>

// FIXME: eradicate ifdef
#ifdef NUI_BACKEND
#    include <nui_basic.hpp>
#else
#    include <emscripten/val.h>
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
    // clang-format on
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