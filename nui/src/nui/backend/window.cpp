#include <nui/window.hpp>

#include <nui/backend/filesystem/special_paths.hpp>
#include <nui/backend/filesystem/file_dialog.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/utility/widen.hpp>

#include <webview.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#if __linux__
#    include <gtk/gtk.h>
#    include <webkit/webkit.h>
#endif

#include <random>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

#if __linux__
extern "C" {
    void resourceLoadStarted(
        WebKitWebView* webView,
        WebKitWebResource* webResource,
        WebKitURIRequest* request,
        gpointer userData)
    {
        std::cout << "resource load started\n";
    }
}
#endif

namespace Nui
{
    // #####################################################################################################################
    struct Window::Implementation
    {
        webview::webview view;
        std::vector<std::filesystem::path> cleanupFiles;
        std::vector<std::function<void(nlohmann::json const&)>> callbacks;
        boost::asio::thread_pool pool{4};

        Implementation(bool debug)
            : view{debug}
        {}
        ~Implementation()
        {
            pool.stop();
            pool.join();
        }
    };
    // #####################################################################################################################
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
    void Window::asyncDispatch(std::function<void()> func)
    {
        boost::asio::post(impl_->pool.executor(), std::move(func));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::string& url)
    {
        impl_->view.navigate(url);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::run()
    {
#ifdef _WIN32
        MSG msg;
        BOOL res;
        while ((res = GetMessage(&msg, nullptr, 0, 0)) != -1)
        {
            if (msg.hwnd)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }
            if (msg.message == WM_APP)
            {
                auto f = reinterpret_cast<std::function<void()>*>(msg.lParam);
                ScopeExit se{[f]() {
                    // yuck! but this is from webview internals
                    delete f;
                }};
                (*f)();
            }
            else if (msg.message == WM_QUIT)
            {
                return;
            }
        }
#else
        impl_->view.run();
#endif
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
                    frontend: {{}}, backend: {{}}, tempId: 0
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
        auto* nativeWebView = static_cast<ICoreWebView2*>(static_cast<webview::browser_engine&>(impl_->view).webview());
        nativeWebView->OpenDevToolsWindow();
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
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setVirtualHostNameToFolderMapping(
        std::string const& hostName,
        std::string const& folderPath,
        HostResourceAccessKind accessKind)
    {
#if defined(_WIN32)
        ICoreWebView2_3* wv23;
        auto* nativeWebView = static_cast<ICoreWebView2*>(static_cast<webview::browser_engine&>(impl_->view).webview());

        std::cout << "QueryInterface\n";
        nativeWebView->QueryInterface(IID_ICoreWebView2_3, reinterpret_cast<void**>(&wv23));

        if (wv23 == nullptr)
            throw std::runtime_error("Could not get interface to set mapping.");

        COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND nativeAccessKind;
        switch (accessKind)
        {
            case (HostResourceAccessKind::Deny):
                nativeAccessKind = COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY;
                break;
            case (HostResourceAccessKind::Allow):
                nativeAccessKind = COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW;
                break;
            case (HostResourceAccessKind::DenyCors):
                nativeAccessKind = COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS;
                break;
        }

        wv23->SetVirtualHostNameToFolderMapping(
            widenString(hostName).c_str(), widenString(folderPath).c_str(), nativeAccessKind);
#elif defined(__APPLE__)
        throw std::runtime_error("Not implemented");
#else
        // TODO:
        auto* nativeWebView = static_cast<ICoreWebView2*>(static_cast<webview::browser_engine&>(impl_->view).webview());
        g_signal_connect(nativeWebView, "resource-load-started", G_CALLBACK(resourceLoadStarted), NULL);
#endif
    }
    // #####################################################################################################################
}