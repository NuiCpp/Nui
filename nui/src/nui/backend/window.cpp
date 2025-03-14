#include <nui/window.hpp>

#include <nui/backend/filesystem/special_paths.hpp>
#include <nui/backend/filesystem/file_dialog.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/utility/widen.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/data_structures/selectables_registry.hpp>
#include <nui/screen.hpp>
#include "load_file.hpp"

#include <webview.h>
#include <roar/mime_type.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#if defined(__APPLE__)
#    include <objc/objc-runtime.h>
#    include <objc/NSObjCRuntime.h>
#    include <CoreGraphics/CoreGraphics.h>
#elif __linux__
#    include <gtk/gtk.h>
#    include <libsoup/soup.h>
#elif defined(_WIN32)
#    if defined(_MSC_VER) && defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wlanguage-extension-token"
#    endif
#endif

#include <random>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <functional>
#include <list>
#include <array>
#include <string>
#include <iostream>

#ifndef _WIN32
namespace Nui
{
    struct HostNameMappingInfo
    {
        std::unordered_map<std::string, std::filesystem::path> hostNameToFolderMapping{};
        std::size_t hostNameMappingMax{0};
    };

    CustomSchemeResponse
    folderMappingResponseFromRequest(CustomSchemeRequest const& req, HostNameMappingInfo const& hostNameMappingInfo)
    {
        using namespace std::string_literals;

        const auto maybeUrl = req.parseUrl();
        if (!maybeUrl)
            return CustomSchemeResponse{.statusCode = 500, .body = "Could not parse url"};

        const auto& url = *maybeUrl;
        const auto hostName = url.hostAsString();

        auto it = hostNameMappingInfo.hostNameToFolderMapping.find(hostName);
        if (it == hostNameMappingInfo.hostNameToFolderMapping.end())
            return CustomSchemeResponse{.statusCode = 404, .body = "Host name not found for mapping"};

        const auto path = url.pathAsString();
        const auto filePath = it->second / std::string{path.data() + 1, path.size() - 1};
        const auto maybeMime = Roar::extensionToMime(filePath.extension().string());
        const auto mime = maybeMime ? *maybeMime : "application/octet-stream";

        const auto body = loadFile(filePath);

        return CustomSchemeResponse{
            .statusCode = body ? 200 : 404,
            .reasonPhrase = body ? "OK" : "Not Found",
            .headers =
                {
                    {"Content-Type"s, "text/plain"s},
                    {"Access-Control-Allow-Origin"s, "*"s},
                    {"Access-Control-Allow-Methods"s, "GET, POST, PUT, DELETE, OPTIONS"s},
                    {"Access-Control-Allow-Headers"s, "*"s},
                    {"Content-Type"s, mime},
                },
            .body = body ? *body : "File not found",
        };
    }
}
#endif

#ifdef __APPLE__
#    include "mac_webview_config_from_window_options.ipp"
#elif defined(_WIN32)
#    include <webview2_environment_options.hpp>
#    ifndef _MSC_VER
#        include <webview2_iids.h>
#    endif
#    include <wrl/event.h>
#    include "environment_options_from_window_options.ipp"
constexpr static auto wakeUpMessage = WM_APP + 1;
#endif

using namespace std::string_literals;

// #####################################################################################################################
namespace Nui
{
    struct Window::Implementation : public std::enable_shared_from_this<Implementation>
    {
        std::recursive_mutex viewGuard;
        boost::asio::thread_pool pool;
        std::unique_ptr<webview::webview> view;
        std::vector<std::filesystem::path> cleanupFiles;
        std::unordered_map<std::string, std::function<void(nlohmann::json const&)>> callbacks;
        int width;
        int height;
        std::function<void(std::string_view)> onRpcError;

        virtual void registerSchemeHandlers(WindowOptions const& options) = 0;

        Implementation()
            : viewGuard{}
            , pool{4}
            , view{}
            , cleanupFiles{}
            , callbacks{}
            , width{0}
            , height{0}
            , onRpcError{}
        {}
        virtual ~Implementation()
        {
            pool.stop();
            pool.join();
        }

        void initialize(bool debug, void* options)
        {
            view = std::make_unique<webview::webview>(debug, nullptr, options);
        }

        template <class Derived>
        std::shared_ptr<Derived> shared_from_base()
        {
            return std::static_pointer_cast<Derived>(shared_from_this());
        }
        template <class Derived>
        std::weak_ptr<Derived> weak_from_base()
        {
            return std::weak_ptr<Derived>(shared_from_base<Derived>());
        }
    };
}
// #####################################################################################################################

#ifdef __APPLE__
#    include "window_impl_mac.ipp"
#elif defined(__linux__)
#    include "window_impl_linux.ipp"
#elif defined(_WIN32)
#    include "window_impl_win.ipp"
#endif

namespace Nui
{
    // #####################################################################################################################
    std::optional<Url> CustomSchemeRequest::parseUrl() const
    {
        auto res = Url::fromString(uri);
        if (!res)
            return std::nullopt;
        return res.value();
    }
    // #####################################################################################################################
    Window::Window()
        : Window{WindowOptions{}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(std::string const& title, bool debug)
        : Window{WindowOptions{
              .title = title,
              .debug = debug,
          }}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(bool debug)
        : Window{WindowOptions{
              .debug = debug,
          }}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(WindowOptions const& options)
#ifdef __APPLE__
        : impl_{std::make_shared<MacOsImplementation>()}
#elif defined(__linux__)
        : impl_{std::make_shared<LinuxImplementation>()}
#elif defined(_WIN32)
        : impl_{std::make_shared<WindowsImplementation>()}
#endif
    {
        if (!options.onRpcError)
        {
            impl_->onRpcError = [](std::string_view msg) {
                std::cerr << "NUI RPC Error: " << msg << std::endl;
            };
        }
        else
            impl_->onRpcError = options.onRpcError;

#ifdef __APPLE__
        impl_->initialize(
            options.debug,
            static_cast<void*>(MacOs::wkWebViewConfigurationFromOptions(
                &static_cast<MacOsImplementation*>(impl_.get())->hostNameMappingInfo, options)));
#elif defined(__linux__)
        impl_->initialize(options.debug, nullptr);
#elif defined(_WIN32)
        impl_->initialize(options.debug, webView2EnvironmentOptionsFromOptions(options).Get());
#endif

        impl_->view->install_message_hook([this](std::string const& msg) {
            std::scoped_lock lock{impl_->viewGuard};
            try
            {
                const auto obj = nlohmann::json::parse(msg);
                if (!obj.contains("id"))
                    return impl_->onRpcError("Message does not contain a callback id!"), false;

                const auto id = obj["id"].get<std::string>();
                auto callbackIter = impl_->callbacks.find(id);
                if (callbackIter == impl_->callbacks.end())
                    return impl_->onRpcError("Callback with id " + id + " does not exist!"), false;

                if (!obj.contains("args"))
                    callbackIter->second(nlohmann::json{});
                else
                    callbackIter->second(obj["args"]);
            }
            catch (std::exception const& exc)
            {
                impl_->onRpcError(
                    "Exception in webview message handler for message: " + msg + "\nException: " + exc.what());
            }
            return false;
        });

        impl_->registerSchemeHandlers(options);

        if (options.title)
            setTitle(*options.title);

#ifdef __APPLE__
#elif defined(__linux__)
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const* title, bool debug)
        : Window{WindowOptions{
              .title = std::string{title},
              .debug = debug,
          }}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window()
    {
        for (auto const& file : impl_->cleanupFiles)
            std::filesystem::remove(file);
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    Window& Window::operator=(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setTitle(std::string const& title)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view->set_title(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->width = width;
        impl_->height = height;
        impl_->view->set_size(width, height, static_cast<int>(hint));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setHtmlThroughFilesystem(std::string_view html)
    {
        std::scoped_lock lock{impl_->viewGuard};
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

        impl_->view->navigate("file://"s + tempFile.string());
        impl_->cleanupFiles.push_back(tempFile);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setHtml(std::string_view html, std::optional<std::string> windowsServeThroughAuthority)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if not defined(_WIN32)
        (void)windowsServeThroughAuthority;
        impl_->view->set_html(std::string{html});
#else
        if (html.size() < 1'572'834 && !windowsServeThroughAuthority)
        {
            impl_->view->set_html(std::string{html});
            return;
        }
        else
        {
            using namespace std::string_literals;

            auto* winImpl = static_cast<WindowsImplementation*>(impl_.get());
            auto* webView = static_cast<ICoreWebView2*>(static_cast<webview::browser_engine&>(*impl_->view).webview());

            if (winImpl->setHtmlWorkaroundToken)
            {
                webView->remove_WebResourceRequested(*winImpl->setHtmlWorkaroundToken);
                winImpl->setHtmlWorkaroundToken.reset();
            }
            winImpl->setHtmlWorkaroundToken.emplace();

            if (!windowsServeThroughAuthority)
                windowsServeThroughAuthority = std::string{windowsServeAuthority};
            const auto authority = *windowsServeThroughAuthority;

            const std::wstring filter = widenString("https://"s + authority + "/index.html");

            auto result = webView->AddWebResourceRequestedFilter(
                filter.c_str(), static_cast<COREWEBVIEW2_WEB_RESOURCE_CONTEXT>(NuiCoreWebView2WebResourceContext::All));

            if (result != S_OK)
                throw std::runtime_error("Could not AddWebResourceRequestedFilter: " + std::to_string(result));

            result = webView->add_WebResourceRequested(
                Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                    [authority,
                     index = std::string{html},
                     winImpl,
                     compareUri = "https://" + authority +
                         "/index.html"](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                        Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> webViewRequest;
                        args->get_Request(&webViewRequest);

                        const auto uri = [&webViewRequest]() {
                            LPWSTR uri;
                            webViewRequest->get_Uri(&uri);
                            std::wstring uriW{uri};
                            CoTaskMemFree(uri);
                            return shortenString(uriW);
                        }();

                        if (uri != compareUri)
                            return S_OK;

                        HRESULT result = 0;

                        auto response = winImpl->makeResponse(
                            CustomSchemeResponse{
                                .statusCode = 200,
                                .reasonPhrase = "OK",
                                .headers =
                                    {
                                        {"Content-Type"s, "text/html"s},
                                        {"Access-Control-Allow-Origin"s, "*"s},
                                        {"Access-Control-Allow-Methods"s, "GET"s},
                                        {"Access-Control-Allow-Headers"s, "*"s},
                                    },
                                .body = index,
                            },
                            result);

                        if (result != S_OK)
                            return result;

                        result = args->put_Response(response.Get());
                        return result;
                    })
                    .Get(),
                &*winImpl->setHtmlWorkaroundToken);

            if (result != S_OK)
                throw std::runtime_error("Could not add_WebResourceRequested: " + std::to_string(result));

            impl_->view->navigate("https://"s + authority + "/index.html");
        }
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(char const* url)
    {
        navigate(std::string{url});
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::string& url)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view->navigate(url);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::filesystem::path& file)
    {
        using namespace std::string_literals;
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view->navigate("file://"s + file.string());
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::dispatch(std::function<void()> func)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view->dispatch(std::move(func));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::run()
    {
#ifdef _WIN32
        auto* winImpl = static_cast<WindowsImplementation*>(impl_.get());
        MSG msg;
        BOOL res;
        while ((res = GetMessage(&msg, nullptr, 0, 0)) != -1)
        {
            {
                std::scoped_lock lock{winImpl->viewGuard};
                if (!winImpl->toProcessOnWindowThread.empty())
                {
                    for (auto const& func : winImpl->toProcessOnWindowThread)
                        func();
                    winImpl->toProcessOnWindowThread.clear();
                }
            }
            if (msg.message == wakeUpMessage)
                continue;
            if (msg.hwnd)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }
            if (msg.message == WM_APP)
            {
                auto* f = reinterpret_cast<std::function<void()>*>(msg.lParam);
                ScopeExit se{[f]() noexcept {
                    // yuck! but this is from webview internals
                    delete f;
                }};
                (*f)();
            }
            else if (msg.message == WM_QUIT)
                return;
        }
#else
        impl_->view->run();
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::terminate()
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view->terminate();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setPosition(int x, int y, bool useFrameOrigin)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        (void)useFrameOrigin;
        SetWindowPos(reinterpret_cast<HWND>(impl_->view->window()), nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
#elif defined(__APPLE__)
        using namespace webview::detail;
        auto wnd = static_cast<id>(impl_->view->window());
        if (useFrameOrigin)
        {
            objc::msg_send<void>(wnd, "setFrameOrigin:"_sel, CGPoint{static_cast<CGFloat>(x), static_cast<CGFloat>(y)});
        }
        else
        {
            objc::msg_send<void>(
                wnd, "setFrameTopLeftPoint:"_sel, CGPoint{static_cast<CGFloat>(x), static_cast<CGFloat>(y)});
        }
#else
        (void)useFrameOrigin;
        gtk_window_move(static_cast<GtkWindow*>(impl_->view->window()), x, y);
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::centerOnPrimaryDisplay()
    {
        std::scoped_lock lock{impl_->viewGuard};
        const auto primaryDisplay = Screen::getPrimaryDisplay();
        setPosition(
            primaryDisplay.x() + (primaryDisplay.width() - impl_->width) / 2,
            primaryDisplay.y() + (primaryDisplay.height() - impl_->height) / 2,
            true);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback)
    {
        if (!callback)
            throw std::runtime_error("Callback must be valid.");

        runInJavascriptThread([this, name, callback]() {
            std::scoped_lock lock{impl_->viewGuard};
            impl_->callbacks[name] = callback;
            auto script = fmt::format(
                R"(
                    (() => {{
                        const name = "{}";
                        const id = "{}";
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
                name);

            impl_->view->init(script);
            impl_->view->eval(script);
        });
    }
    //--------------------------------------------------------------------------------------------------------------------
    void Window::unbind(std::string const& name)
    {
        runInJavascriptThread([this, name]() {
            std::scoped_lock lock{impl_->viewGuard};
            auto script = fmt::format(
                R"(
                    (() => {{
                        const name = "{}";
                        delete globalThis.nui_rpc.backend[name];
                    }})();
                )",
                name);

            impl_->callbacks.erase(name);
            impl_->view->init(script);
            impl_->view->eval(script);
        });
    }
    //--------------------------------------------------------------------------------------------------------------------
    void Window::runInJavascriptThread(std::function<void()>&& func)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        auto* winImpl = static_cast<WindowsImplementation*>(impl_.get());
        if (GetCurrentThreadId() == winImpl->windowThreadId)
            func();
        else
        {
            winImpl->toProcessOnWindowThread.push_back(std::move(func));
            PostThreadMessage(winImpl->windowThreadId, wakeUpMessage, 0, 0);
        }
#else
        func();
#endif
    }
    //--------------------------------------------------------------------------------------------------------------------
    void Window::eval(std::filesystem::path const& file)
    {
        std::scoped_lock lock{impl_->viewGuard};
        const auto script = loadFile(file);
        if (!script)
            throw std::runtime_error("Could not load file: " + file.string());
        impl_->view->eval(*script);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::init(std::string const& js)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view->init(js);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::init(std::filesystem::path const& file)
    {
        std::scoped_lock lock{impl_->viewGuard};
        const auto script = loadFile(file);
        if (!script)
            throw std::runtime_error("Could not load file: " + file.string());
        impl_->view->init(*script);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::eval(char const* js)
    {
        eval(std::string{js});
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::eval(std::string const& js)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        auto* winImpl = static_cast<WindowsImplementation*>(impl_.get());
        if (GetCurrentThreadId() == winImpl->windowThreadId)
        {
            winImpl->view->eval(js);
        }
        else
        {
            winImpl->toProcessOnWindowThread.push_back([js, winImpl]() {
                winImpl->view->eval(js);
            });
            PostThreadMessage(winImpl->windowThreadId, wakeUpMessage, 0, 0);
        }
#elif defined(__APPLE__)
        impl_->view->eval(js);
#else
        impl_->view->eval(js);
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void* Window::getNativeWebView()
    {
        return static_cast<webview::browser_engine&>(*impl_->view).webview();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void* Window::getNativeWindow()
    {
        return impl_->view->window();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::openDevTools()
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        auto* nativeWebView = static_cast<ICoreWebView2*>(getNativeWebView());
        nativeWebView->OpenDevToolsWindow();
#elif defined(__APPLE__)
        // Currently not easily possible.
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

        nativeWebView->QueryInterface(IID_ICoreWebView2_3, reinterpret_cast<void**>(&wv23));

        if (wv23 == nullptr)
            throw std::runtime_error("Could not get interface to set mapping.");
        auto releaseInterface = Nui::ScopeExit{[wv23]() noexcept {
            wv23->Release();
        }};

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

        const auto wideHost = widenString(hostName);
        const auto widePath = widenString(folderPath.string());
        wv23->SetVirtualHostNameToFolderMapping(wideHost.c_str(), widePath.c_str(), nativeAccessKind);
#elif defined(__APPLE__)
        (void)accessKind;
        auto* macImpl = static_cast<MacOsImplementation*>(impl_.get());
        macImpl->hostNameMappingInfo.hostNameMappingMax =
            std::max(macImpl->hostNameMappingInfo.hostNameMappingMax, hostName.size());
        macImpl->hostNameMappingInfo.hostNameToFolderMapping[hostName] = folderPath;
#else
        (void)accessKind;
        auto* linuxImpl = static_cast<LinuxImplementation*>(impl_.get());
        linuxImpl->hostNameMappingInfo.hostNameMappingMax =
            std::max(linuxImpl->hostNameMappingInfo.hostNameMappingMax, hostName.size());
        linuxImpl->hostNameMappingInfo.hostNameToFolderMapping[hostName] = folderPath;
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

#if defined(_MSC_VER) && defined(__clang__)
#    pragma clang diagnostic pop
#endif