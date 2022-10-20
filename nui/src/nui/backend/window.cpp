#include <nui/window.hpp>

#include <nui/backend/filesystem/special_paths.hpp>
#include <nui/backend/filesystem/file_dialog.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/utility/widen.hpp>
#include <nui/utility/scope_exit.hpp>

#include <webview.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <roar/mime_type.hpp>

#if __linux__
#    include <gtk/gtk.h>
#endif

#include <random>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

#if __linux__
struct HostNameMappingInfo
{
    std::unordered_map<std::string, std::filesystem::path> hostNameToFolderMapping{};
    std::size_t hostNameMappingMax{0};
};
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
        std::recursive_mutex viewGuard;
#if __linux__
        HostNameMappingInfo hostNameMappingInfo;
#endif

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
}

#if __linux__
std::size_t strlenLimited(char const* str, std::size_t limit)
{
    std::size_t i = 0;
    while (i <= limit && str[i] != '\0')
        ++i;
    return i;
}

extern "C" {
    // TODO: This function can be improved.
    void uriSchemeRequestCallback(WebKitURISchemeRequest* request, gpointer userData)
    {
        auto* hostNameMappingInfo = static_cast<HostNameMappingInfo*>(userData);

        const auto path = std::string_view{webkit_uri_scheme_request_get_path(request)};
        const auto uri = std::string_view{webkit_uri_scheme_request_get_uri(request)};
        const auto scheme = std::string_view{webkit_uri_scheme_request_get_scheme(request)};

        auto exitError = Nui::ScopeExit{[&] {
            // FIXME: 0, 0: this should be correct error categories.
            auto* error = g_error_new(0, 0, "Invalid custom scheme / Host name mapping.");
            auto freeError = Nui::ScopeExit{[error] {
                g_error_free(error);
            }};
            webkit_uri_scheme_request_finish_error(request, error);
        }};

        auto hostName = std::string{uri.data() + scheme.size() + 3, uri.size() - scheme.size() - 3 - path.size()};
        auto it = hostNameMappingInfo->hostNameToFolderMapping.find(hostName);
        if (it == hostNameMappingInfo->hostNameToFolderMapping.end())
        {
            std::cout << "Host name mapping not found: " << hostName << "\n";
            return;
        }

        const auto filePath = it->second / std::string{path.data() + 1, path.size() - 1};
        auto fileContent = std::string{};
        {
            auto file = std::ifstream{filePath, std::ios::binary};
            if (!file)
            {
                std::cout << "File not found: " << filePath << "\n";
                return;
            }
            file.seekg(0, std::ios::end);
            fileContent.resize(static_cast<std::size_t>(file.tellg()));
            file.seekg(0, std::ios::beg);
            file.read(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
        }

        exitError.disarm();
        GInputStream* stream = g_memory_input_stream_new_from_data(
            g_strdup(fileContent.c_str()), static_cast<gssize>(fileContent.size()), g_free);
        auto freeStream = Nui::ScopeExit{[stream] {
            g_object_unref(stream);
        }};
        const auto maybeMime = Roar::extensionToMime(filePath.extension().string());
        std::string mime = maybeMime ? *maybeMime : "application/octet-stream";
        webkit_uri_scheme_request_finish(request, stream, static_cast<gint64>(fileContent.size()), mime.c_str());
    }

    void uriSchemeDestroyNotify(void*)
    {}
}
#endif

namespace Nui
{
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
    {
        impl_->view.install_message_hook([this](std::string const& msg) {
            std::scoped_lock lock{impl_->viewGuard};
            const auto obj = nlohmann::json::parse(msg);
            impl_->callbacks[obj["id"].get<std::size_t>()](obj["args"]);
            return false;
        });

#if __linux__
        auto nativeWebView = WEBKIT_WEB_VIEW(getNativeWebView());

        auto* webContext = webkit_web_view_get_context(nativeWebView);
        webkit_web_context_register_uri_scheme(
            webContext, "assets", &uriSchemeRequestCallback, &impl_->hostNameMappingInfo, &uriSchemeDestroyNotify);
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const* title, bool debug)
        : Window{std::string{title}, debug}
    {}
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
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.set_title(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        std::scoped_lock lock{impl_->viewGuard};
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

        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.navigate("file://"s + tempFile.string());
        impl_->cleanupFiles.push_back(tempFile);
#else
        std::scoped_lock lock{impl_->viewGuard};
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
        std::scoped_lock lock{impl_->viewGuard};
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
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.terminate();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback)
    {
        std::scoped_lock lock{impl_->viewGuard};
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
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.eval(js);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void* Window::getNativeWebView()
    {
        return static_cast<webview::browser_engine&>(impl_->view).webview();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::openDevTools()
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        auto* nativeWebView = static_cast<ICoreWebView2*>(getNativeWebView());
        nativeWebView->OpenDevToolsWindow();
#elif defined(__APPLE__)
        throw std::runtime_error("Not implemented");
#else
        // FIXME: This freezes the view on Linux when called from there. "received NeedDebuggerBreak trap" in the
        // console, Is a breakpoint auto set and locks everything up?
        auto nativeWebView = WEBKIT_WEB_VIEW(getNativeWebView());
        auto webkitInspector = webkit_web_view_get_inspector(nativeWebView);
        webkit_web_inspector_show(webkitInspector);
        webkit_web_inspector_detach(webkitInspector);
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setVirtualHostNameToFolderMapping(
        std::string const& hostName,
        std::filesystem::path const& folderPath,
        HostResourceAccessKind accessKind)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        ICoreWebView2_3* wv23;
        auto* nativeWebView = static_cast<ICoreWebView2*>(getNativeWebView());

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
            widenString(hostName).c_str(), widenString(folderPath.string()).c_str(), nativeAccessKind);
#elif defined(__APPLE__)
        throw std::runtime_error("Not implemented");
#else
        (void)accessKind;
        impl_->hostNameMappingInfo.hostNameMappingMax =
            std::max(impl_->hostNameMappingInfo.hostNameMappingMax, hostName.size());
        impl_->hostNameMappingInfo.hostNameToFolderMapping[hostName] = folderPath;
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setConsoleOutput(bool active)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if __linux__
        auto nativeWebView = WEBKIT_WEB_VIEW(getNativeWebView());
        auto* settings = webkit_web_view_get_settings(nativeWebView);
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, active);
        webkit_web_view_set_settings(nativeWebView, settings);
#else
        (void)active;
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::any_io_executor Window::getExecutor() const
    {
        return impl_->pool.executor();
    }
    // #####################################################################################################################
}