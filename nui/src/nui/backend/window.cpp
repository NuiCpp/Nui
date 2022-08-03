#include <nui/window.hpp>

#include <fmt/format.h>
#include <webview.h>

namespace Nui
{
    namespace
    {
        constexpr const auto htmlTemplate = R"NUIX(
        <div id="root">hello</div>
        <script>{}</script>)NUIX";
    }
    //#####################################################################################################################
    struct Window::Implementation
    {
        webview::webview view;

        Implementation(bool debug)
            : view{debug}
        {}
    };
    //#####################################################################################################################
    Window::Window()
        : Window{false}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(std::string const& title, bool debug)
        : Window{debug}
    {
        setTitle(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(bool debug)
        : impl_{std::make_unique<Implementation>(debug)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const* title, bool debug)
        : Window{std::string{title}, debug}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window() = default;
    Window::Window(Window&&) = default;
    Window& Window::operator=(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setTitle(std::string const& title)
    {
        impl_->view.set_title(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        impl_->view.set_size(width, height, static_cast<int>(hint));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::loadFrontend(std::string_view front)
    {
        impl_->view.set_html(fmt::format(htmlTemplate, front));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::string& url)
    {
        impl_->view.navigate(url);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::run()
    {
        impl_->view.run();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::terminate()
    {
        impl_->view.terminate();
    }
    //#####################################################################################################################
}