#include <nui/core.hpp>
#include <nui/window.hpp>

// Generated file by emscripten that is the page. The header name is the target name.
#include <nui_basic.hpp>

#include <iostream>
#include <string_view>
#include <functional>

using namespace Nui;

int main()
{
    Window window{"Basic Example"};
    window.setSize(480, 320, Nui::WebViewHint::WEBVIEW_HINT_NONE);

    window.loadFrontend(nui_basic());
    window.run();

    return 0;
}