#include <nui/window.hpp>

#include <nui/backend/filesystem/special_paths.hpp>

#include <webview.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <random>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

namespace Nui
{
    //#####################################################################################################################
    struct Window::Implementation
    {
        webview::webview view;
        std::vector<std::filesystem::path> cleanupFiles;
        std::vector<std::function<void(nlohmann::json const&)>> callbacks;

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
    {
        impl_->view.install_message_hook([this](std::string const& msg) {
            const auto obj = nlohmann::json::parse(msg);
            impl_->callbacks[obj["id"].get<std::size_t>()](obj["args"]);
            return false;
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window()
    {
        for (auto const& file : impl_->cleanupFiles)
            std::filesystem::remove(file);
    }
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
    void Window::setHtml(std::string_view html)
    {
#if defined(_WIN32)
        // https://github.com/MicrosoftEdge/WebView2Feedback/issues/1355
        // :((((

        using namespace std::string_literals;
        constexpr static auto fileNameSize = 25;
        std::string_view alphanum =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<std::size_t> dis(0, alphanum.size() - 1);
        std::string fileName(fileNameSize, '\0');
        for (std::size_t i = 0; i < fileNameSize; ++i)
            fileName[i] = alphanum[dis(gen)];
        const auto tempFile = resolvePath("%temp%/"s + fileName + ".html");
        {
            std::ofstream temporary{tempFile, std::ios_base::binary};
            temporary.write(html.data(), static_cast<std::streamsize>(html.size()));
        }
        impl_->view.navigate("file://"s + tempFile.string());
        impl_->cleanupFiles.push_back(tempFile);
#else
        impl_->view.set_html(std::string{html});
#endif
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
    //---------------------------------------------------------------------------------------------------------------------
    void Window::bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback)
    {
        impl_->callbacks.push_back(callback);
        auto script = fmt::format(
            R"(
            (() => {{
                const name = "{}";
                const id = {};
                globalThis.nui_rpc = (globalThis.nui_rpc || {{
                    frontend: {{}}, backend: {{}}
                }});
                globalThis.nui_rpc.backend[name] = (...args) => {{
                    globalThis.external.invoke(JSON.stringify({{
                        name: name,
                        id: id,
                        args: [...args]
                    }}))  
                }};
            }})();
        )",
            name,
            impl_->callbacks.size() - 1);
        impl_->view.init(script);
        impl_->view.eval(script);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::eval(std::string const& js)
    {
        impl_->view.eval(js);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::openDevTools()
    {
#if defined(_WIN32)
        auto* nativeWindowHandle = static_cast<ICoreWebView2*>(impl_->view.window());
        // FIXME:
        // nativeWindowHandle->OpenDevToolsWindow();
        throw std::runtime_error("Not implemented");
#elif defined(__APPLE__)
        throw std::runtime_error("Not implemented");
#else
        auto gtkWindow{GTK_WINDOW(w.window())};
        auto children{gtk_container_get_children(GTK_CONTAINER(gtkWindow))};
        auto gtkWidget{GTK_WIDGET(children[0].data)};
        g_list_free(children);
        auto webkitWebView{WEBKIT_WEB_VIEW(gtkWidget)};
        // Unnecessary because because webview calls webkit_settings_set_enable_developer_extras
        // auto webkitSettings{webkit_web_view_get_settings(webkitWebView)};
        // g_object_set(G_OBJECT(webkitSettings), "enable-developer-extras", TRUE, NULL);
        auto webkitInspector{webkit_web_view_get_inspector(webkitWebView)};
        webkit_web_inspector_show(webkitInspector);
#endif
    }
    //#####################################################################################################################
}