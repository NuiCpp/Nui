#pragma once

#include <memory>
#include <string>

namespace Nui
{
    enum class WebViewHint : int
    {
        WEBVIEW_HINT_NONE,
        WEBVIEW_HINT_MIN,
        WEBVIEW_HINT_MAX,
        WEBVIEW_HINT_NIXED
    };

    class Window
    {
      public:
        Window();
        explicit Window(bool debug);
        explicit Window(std::string const& title, bool debug = false);
        explicit Window(char const* title, bool debug = false);
        ~Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&);
        Window& operator=(Window&&);

        void setTitle(std::string const& title);
        void setSize(int width, int height, WebViewHint hint);
        void navigate(const std::string& url);
        void run();
        void terminate();
        void loadFrontend(std::string_view front);
        // eval
        // init
        // bind
        // unbind
        // dispatch
        // set_html

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}