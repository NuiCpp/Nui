#include <nui/window.hpp>

#include <nui/frontend/rpc_client.hpp>

namespace Nui
{
    // #####################################################################################################################
    struct Window::Implementation
    {
        Implementation(bool)
        {}
    };
    // #####################################################################################################################
    Window::Window()
        : Window{false}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(std::string const&, bool debug)
        : Window{debug}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(bool debug)
        : impl_{std::make_unique<Implementation>(debug)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const*, bool debug)
        : Window{debug}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window() = default;
    Window::Window(Window&&) = default;
    Window& Window::operator=(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setTitle(std::string const& title)
    {
        RpcClient::getRemoteCallable("nui_setWindowTitle")(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        RpcClient::getRemoteCallable("nui_setWindowSize")(width, height, static_cast<int>(hint));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::string&)
    {
        // TODO: something smarter here. maybe some "react router" kind of thing?
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::terminate()
    {
        RpcClient::getRemoteCallable("nui_terminate")();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::openDevTools()
    {
        RpcClient::getRemoteCallable("nui_openDevTools")();
    }
    // #####################################################################################################################
}