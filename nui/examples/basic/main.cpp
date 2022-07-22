#include <nui/window.hpp>

#include <iostream>

constexpr const auto html = R"NUIX(
<div id="root"></div>
<script>
    const initialize = () => {
        const root = document.querySelectorAll("#root")[0];
        const btn = document.createElement("button");
        btn.onclick = window.increment;
        root.appendChild(btn);
    };
    initialize();
</script>)NUIX";

int main()
{
    Nui::Window window{"Basic Example"};
    window.setSize(480, 320, Nui::WebViewHint::WEBVIEW_HINT_NONE);

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