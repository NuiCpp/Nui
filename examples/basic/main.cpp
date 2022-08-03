#include <nui/core.hpp>
#include <nui/window.hpp>

#ifndef NUI_FRONTEND
#    include <nui_basic.hpp>
#endif

#include <iostream>
#include <string_view>

int EMSCRIPTEN_KEEPALIVE main()
{
    Nui::Window window{"Basic Example"};
    window.setSize(480, 320, Nui::WebViewHint::WEBVIEW_HINT_NONE);

#ifndef NUI_FRONTEND
    window.loadFrontend(nui_basic());
#endif

    // w.bind("increment", [&](const std::string& s) -> std::string {
    //     auto count_string = std::to_string(++count);
    //     std::cout << count_string << "\n";
    //     return "{\"count\": " + count_string + "}";
    // });
    // w.set_html(html);
    // w.run();

    window.run();

    return 0;
}