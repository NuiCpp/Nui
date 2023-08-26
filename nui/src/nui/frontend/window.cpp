#include <nui/window.hpp>

#include <nui/frontend/rpc_client.hpp>

namespace Nui
{
    // #####################################################################################################################
    struct Window::Implementation
    {
        Implementation()
        {}
    };
    // #####################################################################################################################
    Window::Window()
        : Window{WindowOptions{}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(WindowOptions const&)
        : impl_{std::make_unique<Implementation>()}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(std::string const&, bool)
        : Window{WindowOptions{}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(bool)
        : Window{WindowOptions{}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const*, bool)
        : Window{WindowOptions{}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window() = default;
    Window::Window(Window&&) = default;
    Window& Window::operator=(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setTitle(std::string const& title)
    {
        RpcClient::getRemoteCallable("Nui::setWindowTitle")(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        RpcClient::getRemoteCallable("Nui::setWindowSize")(width, height, static_cast<int>(hint));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::string& location)
    {
        RpcClient::getRemoteCallable("Nui::navigate")(location);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::terminate()
    {
        RpcClient::getRemoteCallable("Nui::terminate")();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::openDevTools()
    {
        RpcClient::getRemoteCallable("Nui::openDevTools")();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setPosition(int x, int y)
    {
        RpcClient::getRemoteCallable("Nui::setPosition")(x, y);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::centerOnPrimaryDisplay()
    {
        RpcClient::getRemoteCallable("Nui::centerOnPrimaryDisplay")();
    }
    // #####################################################################################################################
}