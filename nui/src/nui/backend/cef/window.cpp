#include "client.hpp"
#include "engine_state.hpp"
#include "task.hpp"
#include "window_shared.hpp"
#include "common/messages.hpp"
#ifdef __APPLE__
#    include "mac/application_mac.hpp"
#endif
#include "../load_file.hpp"
#include "../rpc_message.hpp"

#include <nui/cef/client_hooks.hpp>
#include <nui/window.hpp>

#include <roar/mime_type.hpp>
#include <fmt/format.h>
#include <boost/asio/thread_pool.hpp>

#include <include/cef_command_line.h>
#include <include/cef_parser.h>
#include <include/views/cef_browser_view.h>
#include <include/views/cef_window.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nui
{
    namespace
    {
        constexpr char const* defaultHtmlAuthority = "app.example";

        struct WindowGeometry
        {
            std::int32_t width{640};
            std::int32_t height{480};
            WebViewHint hint{WebViewHint::WEBVIEW_HINT_NONE};
            std::optional<CefPoint> position{};
            bool center{false};
            std::string title{};
        };

        std::optional<std::filesystem::path>
        mappedFile(std::filesystem::path const& folder, std::string const& encodedPath)
        {
            std::filesystem::path relative;
            std::string_view rest{encodedPath};
            while (!rest.empty())
            {
                const auto slash = rest.find('/');
                const auto encoded = rest.substr(0, slash);
                rest = slash == std::string_view::npos ? std::string_view{} : rest.substr(slash + 1);
                if (encoded.empty())
                    continue;
                const auto segment = CefURIDecode(
                                         std::string{encoded},
                                         true,
                                         static_cast<cef_uri_unescape_rule_t>(
                                             UU_SPACES | UU_PATH_SEPARATORS |
                                             UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS))
                                         .ToString();
                if (segment == "." || segment == ".." || segment.find_first_of(std::string_view{"/\\\0", 3}) !=
                        std::string::npos)
                    return std::nullopt;
                relative /= segment;
            }
            std::error_code error;
            const auto root = std::filesystem::weakly_canonical(folder, error);
            const auto file = std::filesystem::weakly_canonical(folder / relative, error);
            if (error)
                return std::nullopt;
            const auto [rootEnd, fileIter] = std::mismatch(root.begin(), root.end(), file.begin(), file.end());
            if (rootEnd != root.end())
                return std::nullopt;
            return file;
        }

        CustomScheme folderMappingScheme(std::string scheme, std::weak_ptr<Detail::Cef::WindowShared> weakShared)
        {
            using namespace std::string_literals;
            return CustomScheme{
                .scheme = std::move(scheme),
                .allowedOrigins = {"*"},
                .onRequest =
                    [weakShared](CustomSchemeRequest const& request) {
                        const auto shared = weakShared.lock();
                        if (!shared)
                            return CustomSchemeResponse{.statusCode = 404, .body = "Window is closed"};
                        CefURLParts parts;
                        if (!CefParseURL(request.uri, parts))
                            return CustomSchemeResponse{.statusCode = 500, .body = "Could not parse url"};
                        const auto host = CefString(&parts.host).ToString();
                        std::optional<std::filesystem::path> folder;
                        {
                            std::shared_lock lock{shared->folderMappingGuard};
                            if (const auto iter = shared->folderMappings.find(host); iter != shared->folderMappings.end())
                                folder = iter->second;
                        }
                        if (!folder)
                            return CustomSchemeResponse{.statusCode = 404, .body = "Host name not found for mapping"};
                        const auto file = mappedFile(*folder, CefString(&parts.path).ToString());
                        std::error_code error;
                        if (!file || !std::filesystem::is_regular_file(*file, error))
                            return CustomSchemeResponse{.statusCode = 404, .reasonPhrase = "Not Found", .body = "File not found"};
                        const auto mime = Roar::extensionToMime(file->extension().string());
                        return CustomSchemeResponse{
                            .statusCode = 200,
                            .reasonPhrase = "OK",
                            .headers =
                                {
                                    {"Access-Control-Allow-Origin"s, "*"s},
                                    {"Access-Control-Allow-Methods"s, "GET, POST, PUT, DELETE, OPTIONS"s},
                                    {"Access-Control-Allow-Headers"s, "*"s},
                                    {"Content-Type"s, mime ? *mime : "application/octet-stream"s},
                                },
                            .bodyFile = *file,
                        };
                    },
                .handlerThread = SchemeHandlerThread::Any,
            };
        }
    }

    // #####################################################################################################################
    struct Window::Implementation
        : public Detail::Cef::BrowserEvents
        , public std::enable_shared_from_this<Implementation>
    {
        enum class BrowserState
        {
            NotRequested,
            Pending,
            Created,
            Closed,
        };

        std::shared_ptr<Detail::Cef::WindowShared> shared{std::make_shared<Detail::Cef::WindowShared>()};
        boost::asio::thread_pool pool{4};

        mutable std::mutex mutex{};
        std::unordered_map<std::string, Detail::RpcCallback> callbacks{};
        std::function<void(std::string_view)> onRpcError{};
        std::function<void()> onRpcAliveMessage{};
        std::vector<std::string> initScripts{};
        std::vector<std::string> appSchemes{};
        std::vector<CefRefPtr<CefProcessMessage>> outgoing{};
        bool flushScheduled{false};
        BrowserState state{BrowserState::NotRequested};
        bool countedOpen{false};
        bool terminated{false};
        bool destroyed{false};
        std::string targetUrl{};
        std::optional<std::string> pendingNavigation{};
        WindowGeometry geometry{};
        bool debug{false};
        bool showConsoleOutput{true};
        Nui::Cef::ClientHooks hooks{};
        std::size_t droppedMessages{0u};

        // UI thread only.
        CefRefPtr<CefBrowser> browser{};
        CefRefPtr<CefBrowserView> browserView{};
        CefRefPtr<CefWindow> cefWindow{};

        void createBrowser();
        void send(std::function<CefRefPtr<CefProcessMessage>()> const& makeMessage);
        void flush();
        void addScript(std::string script);
        void closeFromUi();
        void reportClosed();
        void withWindow(std::function<void(CefRefPtr<CefWindow> const&)> action);

        void onAfterCreated(CefRefPtr<CefBrowser> created) override;
        void onBeforeClose(CefRefPtr<CefBrowser> closing) override;
        void onPost(std::string_view message) override;
        bool isAppUrl(std::string const& url) const override;
        bool consoleOutput() const override;
    };

    namespace Detail
    {
        struct WindowAccess
        {
            static Window::Implementation& of(Window& window)
            {
                return *window.impl_;
            }
        };
    }

    namespace
    {
        cef_runtime_style_t runtimeStyle()
        {
            return CEF_RUNTIME_STYLE_ALLOY;
        }

        class BrowserViewDelegate : public CefBrowserViewDelegate
        {
          public:
            cef_runtime_style_t GetBrowserRuntimeStyle() override
            {
                return runtimeStyle();
            }

          private:
            IMPLEMENT_REFCOUNTING(BrowserViewDelegate);
        };

        class WindowDelegate : public CefWindowDelegate
        {
          public:
            WindowDelegate(CefRefPtr<CefBrowserView> browserView, std::weak_ptr<Window::Implementation> window)
                : browserView_{std::move(browserView)}
                , window_{std::move(window)}
            {}

            void OnWindowCreated(CefRefPtr<CefWindow> window) override
            {
                window->AddChildView(browserView_);
                const auto geometry = currentGeometry();
                window->SetTitle(geometry.title);
                if (geometry.position)
                    window->SetPosition(*geometry.position);
                else if (geometry.center)
                    window->CenterWindow(CefSize{geometry.width, geometry.height});
                window->Show();
                browserView_->RequestFocus();
            }

            void OnWindowDestroyed(CefRefPtr<CefWindow>) override
            {
                browserView_ = nullptr;
            }

            bool CanClose(CefRefPtr<CefWindow>) override
            {
                if (!browserView_)
                    return true;
                const auto browser = browserView_->GetBrowser();
                return !browser || browser->GetHost()->TryCloseBrowser();
            }

            CefSize GetPreferredSize(CefRefPtr<CefView>) override
            {
                const auto geometry = currentGeometry();
                return CefSize{geometry.width, geometry.height};
            }

            CefSize GetMinimumSize(CefRefPtr<CefView>) override
            {
                const auto geometry = currentGeometry();
                if (geometry.hint == WebViewHint::WEBVIEW_HINT_MIN || geometry.hint == WebViewHint::WEBVIEW_HINT_FIXED)
                    return CefSize{geometry.width, geometry.height};
                return CefSize{};
            }

            CefSize GetMaximumSize(CefRefPtr<CefView>) override
            {
                const auto geometry = currentGeometry();
                if (geometry.hint == WebViewHint::WEBVIEW_HINT_MAX || geometry.hint == WebViewHint::WEBVIEW_HINT_FIXED)
                    return CefSize{geometry.width, geometry.height};
                return CefSize{};
            }

            bool CanResize(CefRefPtr<CefWindow>) override
            {
                return currentGeometry().hint != WebViewHint::WEBVIEW_HINT_FIXED;
            }

            cef_runtime_style_t GetWindowRuntimeStyle() override
            {
                return runtimeStyle();
            }

#ifndef __APPLE__
            bool GetLinuxWindowProperties(CefRefPtr<CefWindow>, CefLinuxWindowProperties& properties) override
            {
                const auto& identity = Detail::Cef::EngineState::instance().desktopApplicationId();
                if (identity.empty())
                    return false;
                auto className = identity;
                className.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(className.front())));
                CefString(&properties.wayland_app_id) = identity;
                CefString(&properties.wm_class_name) = identity;
                CefString(&properties.wm_class_class) = className;
                return true;
            }
#endif

          private:
            WindowGeometry currentGeometry() const
            {
                if (const auto window = window_.lock())
                {
                    std::scoped_lock lock{window->mutex};
                    return window->geometry;
                }
                return WindowGeometry{};
            }

            CefRefPtr<CefBrowserView> browserView_;
            std::weak_ptr<Window::Implementation> window_;
            IMPLEMENT_REFCOUNTING(WindowDelegate);
        };

        std::function<void()> gated(std::weak_ptr<Detail::Cef::WindowShared> weakShared, std::function<void()> function)
        {
            return [weakShared = std::move(weakShared), function = std::move(function)]() {
                const auto shared = weakShared.lock();
                if (!shared)
                    return;
                Detail::Cef::GateEntry entry{shared->gate};
                if (entry)
                    function();
            };
        }
    }
    // #####################################################################################################################
    void Window::Implementation::createBrowser()
    {
        auto extraInfo = CefDictionaryValue::Create();
        std::string url;
        Nui::Cef::ClientHooks clientHooks;
        {
            std::scoped_lock lock{mutex};
            if (state != BrowserState::NotRequested)
                return;
            if (terminated || destroyed)
            {
                state = BrowserState::Closed;
                reportClosed();
                return;
            }
            state = BrowserState::Pending;
            auto scripts = CefListValue::Create();
            for (std::size_t index = 0u; index != initScripts.size(); ++index)
                scripts->SetString(index, initScripts[index]);
            extraInfo->SetList(Detail::Cef::initScriptsKey, scripts);
            auto schemes = CefListValue::Create();
            for (std::size_t index = 0u; index != appSchemes.size(); ++index)
                schemes->SetString(index, appSchemes[index]);
            extraInfo->SetList(Detail::Cef::appSchemesKey, schemes);
            auto hosts = CefListValue::Create();
            {
                std::shared_lock htmlLock{shared->htmlGuard};
                hosts->SetString(0, shared->htmlAuthority);
            }
            extraInfo->SetList(Detail::Cef::appHostsKey, hosts);
            url = targetUrl.empty() ? std::string{"about:blank"} : targetUrl;
            clientHooks = hooks;
        }

        CefRefPtr<Detail::Cef::Client> client = new Detail::Cef::Client(shared_from_this(), std::move(clientHooks));
        auto view = CefBrowserView::CreateBrowserView(
            client, url, CefBrowserSettings{}, extraInfo, nullptr, new BrowserViewDelegate());
        auto window = CefWindow::CreateTopLevelWindow(new WindowDelegate(view, weak_from_this()));
        std::scoped_lock lock{mutex};
        browserView = view;
        cefWindow = window;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::reportClosed()
    {
        // Called with mutex held.
        if (!countedOpen)
            return;
        countedOpen = false;
        Detail::Cef::EngineState::instance().windowClosed();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::onAfterCreated(CefRefPtr<CefBrowser> created)
    {
        Detail::Cef::EngineState::instance().registerBrowser(created->GetIdentifier(), shared);
        bool closeNow = false;
        std::optional<std::string> navigation;
        {
            std::scoped_lock lock{mutex};
            browser = created;
            state = BrowserState::Created;
            closeNow = terminated || destroyed;
            navigation = std::exchange(pendingNavigation, std::nullopt);
        }
        if (closeNow)
        {
            created->GetHost()->CloseBrowser(true);
            return;
        }
        if (navigation)
            created->GetMainFrame()->LoadURL(*navigation);
        flush();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::onBeforeClose(CefRefPtr<CefBrowser> closing)
    {
        Detail::Cef::EngineState::instance().unregisterBrowser(closing->GetIdentifier());
        bool wasCounted = false;
        {
            std::scoped_lock lock{mutex};
            state = BrowserState::Closed;
            browser = nullptr;
            browserView = nullptr;
            cefWindow = nullptr;
            outgoing.clear();
            wasCounted = std::exchange(countedOpen, false);
        }
        if (wasCounted)
            Detail::Cef::EngineState::instance().windowClosed();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::onPost(std::string_view message)
    {
        Detail::Cef::GateEntry entry{shared->gate};
        if (!entry)
            return;
        std::function<void(std::string_view)> errorHandler;
        std::function<void()> aliveHandler;
        {
            std::scoped_lock lock{mutex};
            errorHandler = onRpcError;
            aliveHandler = onRpcAliveMessage;
        }
        Detail::dispatchRpcMessage(
            message,
            Detail::RpcMessageHandlers{
                .findCallback =
                    [this](std::string const& id) {
                        std::scoped_lock lock{mutex};
                        const auto callback = callbacks.find(id);
                        return callback == callbacks.end() ? Detail::RpcCallback{} : callback->second;
                    },
                .onRpcAlive = aliveHandler,
                .onRpcError = errorHandler,
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Window::Implementation::isAppUrl(std::string const& url) const
    {
        CefURLParts parts;
        if (!CefParseURL(url, parts))
            return false;
        const auto scheme = CefString(&parts.scheme).ToString();
        if (scheme == "file")
            return true;
        {
            std::scoped_lock lock{mutex};
            if (std::ranges::find(appSchemes, scheme) != appSchemes.end())
                return true;
        }
        std::shared_lock lock{shared->htmlGuard};
        return scheme == "https" && CefString(&parts.host).ToString() == shared->htmlAuthority;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Window::Implementation::consoleOutput() const
    {
        std::scoped_lock lock{mutex};
        return showConsoleOutput;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::send(std::function<CefRefPtr<CefProcessMessage>()> const& makeMessage)
    {
        const auto accepting = [this]() {
            return state != BrowserState::NotRequested && state != BrowserState::Closed && !terminated && !destroyed;
        };
        {
            std::scoped_lock lock{mutex};
            if (!accepting())
            {
                // Like the native engines, nothing reaches a view that has no page yet.
                ++droppedMessages;
                return;
            }
        }
        // Built outside the lock, large payloads are copied into shared memory here. No CEF object may be created
        // once the engine shut down.
        if (Detail::Cef::EngineState::instance().finished())
            return;
        auto message = makeMessage();
        {
            std::scoped_lock lock{mutex};
            if (!accepting())
                return;
            outgoing.push_back(std::move(message));
            if (flushScheduled)
                return;
            flushScheduled = true;
        }
        Detail::Cef::EngineState::instance().postToUi([weak = weak_from_this()]() {
            if (const auto self = weak.lock())
                self->flush();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::flush()
    {
        std::vector<CefRefPtr<CefProcessMessage>> messages;
        CefRefPtr<CefBrowser> target;
        {
            std::scoped_lock lock{mutex};
            flushScheduled = false;
            if (!browser)
                return;
            messages = std::move(outgoing);
            outgoing.clear();
            target = browser;
        }
        const auto frame = target->GetMainFrame();
        if (!frame)
            return;
        for (auto const& message : messages)
            frame->SendProcessMessage(PID_RENDERER, message);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::addScript(std::string script)
    {
        bool sendNow = false;
        {
            std::scoped_lock lock{mutex};
            initScripts.push_back(script);
            sendNow = state == BrowserState::Pending || state == BrowserState::Created;
        }
        if (sendNow)
        {
            send([&script]() {
                return Detail::Cef::makeTextMessage(Detail::Cef::addInitScriptMessageName, script);
            });
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::closeFromUi()
    {
        CefRefPtr<CefBrowser> target;
        {
            std::scoped_lock lock{mutex};
            terminated = true;
            if (state == BrowserState::NotRequested)
            {
                state = BrowserState::Closed;
                reportClosed();
                return;
            }
            target = browser;
        }
        // A pending browser closes in onAfterCreated.
        if (target)
            target->GetHost()->CloseBrowser(true);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::Implementation::withWindow(std::function<void(CefRefPtr<CefWindow> const&)> action)
    {
        Detail::Cef::EngineState::instance().postToUi([weak = weak_from_this(), action = std::move(action)]() {
            const auto self = weak.lock();
            if (!self)
                return;
            CefRefPtr<CefWindow> window;
            {
                std::scoped_lock lock{self->mutex};
                window = self->cefWindow;
            }
            if (window)
                action(window);
        });
    }
    // #####################################################################################################################
    std::optional<Url> CustomSchemeRequest::parseUrl() const
    {
        auto result = Url::fromString(uri);
        if (!result)
            return std::nullopt;
        return result.value();
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window()
        : Window{WindowOptions{}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(std::string const& title, bool debug)
        : Window{WindowOptions{.title = title, .debug = debug}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(bool debug)
        : Window{WindowOptions{.debug = debug}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const* title, bool debug)
        : Window{WindowOptions{.title = std::string{title}, .debug = debug}}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(WindowOptions const& options)
        : impl_{std::make_shared<Implementation>()}
    {
        auto& engine = Detail::Cef::EngineState::instance();

        std::vector<std::string> schemeNames;
        for (auto const& scheme : options.customSchemes)
        {
            if (std::ranges::find(schemeNames, scheme.scheme) != schemeNames.end())
                throw std::invalid_argument("Nui: custom scheme '" + scheme.scheme + "' is registered twice");
            schemeNames.push_back(scheme.scheme);
        }
        const bool mapFolders =
            options.folderMappingScheme && std::ranges::find(schemeNames, *options.folderMappingScheme) == schemeNames.end();
        if (mapFolders)
            schemeNames.push_back(*options.folderMappingScheme);

        engine.registerWindowRequirements(schemeNames, defaultHtmlAuthority, options.debug, options.browserArguments);

        for (auto const& scheme : options.customSchemes)
            impl_->shared->schemes.emplace(scheme.scheme, std::make_shared<CustomScheme const>(scheme));
        if (mapFolders)
        {
            impl_->shared->schemes.emplace(
                *options.folderMappingScheme,
                std::make_shared<CustomScheme const>(folderMappingScheme(*options.folderMappingScheme, impl_->shared)));
        }
        impl_->shared->htmlAuthority = defaultHtmlAuthority;
        impl_->appSchemes = schemeNames;
        impl_->onRpcError = options.onRpcError ? options.onRpcError : [](std::string_view message) {
            std::cerr << "NUI RPC Error: " << message << std::endl;
        };
        impl_->onRpcAliveMessage = options.onRpcAliveMessage;
        impl_->debug = options.debug;
        if (options.title)
            impl_->geometry.title = *options.title;

        impl_->shared->requestClose = [weak = std::weak_ptr<Implementation>{impl_}]() {
            if (const auto self = weak.lock())
                self->closeFromUi();
        };
        impl_->countedOpen = true;
        engine.windowOpened([weak = std::weak_ptr<Implementation>{impl_}]() {
            if (const auto self = weak.lock())
                self->createBrowser();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window()
    {
        if (!impl_)
            return;
        auto& engine = Detail::Cef::EngineState::instance();
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->destroyed = true;
        }
        impl_->pool.stop();
        impl_->pool.join();

        if (engine.started())
        {
            if (engine.onUiThread())
                impl_->closeFromUi();
            else
                engine.postToUi([impl = impl_]() {
                    impl->closeFromUi();
                });
        }
        else
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->state = Implementation::BrowserState::Closed;
            if (impl_->countedOpen && !engine.finished())
            {
                impl_->countedOpen = false;
                engine.windowClosed();
            }
        }

        // Engine threads may still be inside a scheme handler or a message callback of this window.
        const auto ownEntries = Detail::Cef::GateEntry::heldByCurrentThread(impl_->shared->gate);
        if (!impl_->shared->gate.closeAndWait(std::chrono::seconds{5}, ownEntries))
            std::cerr << "Nui: window destroyed while engine calls into it did not finish" << std::endl;
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    Window& Window::operator=(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setTitle(std::string const& title)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->geometry.title = title;
        }
        impl_->withWindow([title](CefRefPtr<CefWindow> const& window) {
            window->SetTitle(title);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(std::int32_t width, std::int32_t height, WebViewHint hint)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->geometry.width = width;
            impl_->geometry.height = height;
            impl_->geometry.hint = hint;
        }
        if (hint == WebViewHint::WEBVIEW_HINT_NONE || hint == WebViewHint::WEBVIEW_HINT_FIXED)
        {
            impl_->withWindow([width, height](CefRefPtr<CefWindow> const& window) {
                window->SetSize(CefSize{width, height});
            });
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setPosition(std::int32_t x, std::int32_t y, bool)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->geometry.position = CefPoint{x, y};
        }
        impl_->withWindow([x, y](CefRefPtr<CefWindow> const& window) {
            window->SetPosition(CefPoint{x, y});
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::centerOnPrimaryDisplay()
    {
        std::int32_t width = 0;
        std::int32_t height = 0;
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->geometry.center = true;
            impl_->geometry.position.reset();
            width = impl_->geometry.width;
            height = impl_->geometry.height;
        }
        impl_->withWindow([width, height](CefRefPtr<CefWindow> const& window) {
            window->CenterWindow(CefSize{width, height});
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(char const* url)
    {
        navigate(std::string{url});
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(std::string const& url)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->state == Implementation::BrowserState::NotRequested)
            {
                impl_->targetUrl = url;
                return;
            }
            if (impl_->state != Implementation::BrowserState::Created)
            {
                impl_->pendingNavigation = url;
                return;
            }
        }
        Detail::Cef::EngineState::instance().postToUi([weak = std::weak_ptr<Implementation>{impl_}, url]() {
            const auto self = weak.lock();
            if (!self)
                return;
            CefRefPtr<CefBrowser> browser;
            {
                std::scoped_lock lock{self->mutex};
                browser = self->browser;
            }
            if (browser)
                browser->GetMainFrame()->LoadURL(url);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(std::filesystem::path const& file)
    {
        navigate("file://" + file.string());
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setHtml(std::string_view html, std::optional<std::string> windowsServeThroughAuthority)
    {
        const auto authority = windowsServeThroughAuthority.value_or(std::string{defaultHtmlAuthority});
        Detail::Cef::EngineState::instance().ensureHtmlAuthority(authority);
        {
            std::unique_lock lock{impl_->shared->htmlGuard};
            impl_->shared->html = std::make_shared<std::string const>(html);
            impl_->shared->htmlAuthority = authority;
        }
        navigate(fmt::format("https://{}/index.html", authority));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setHtmlThroughFilesystem(std::string_view html)
    {
        setHtml(html);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::dispatch(std::function<void()> func)
    {
        Detail::Cef::EngineState::instance().postToUi(gated(impl_->shared, std::move(func)));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::runInJavascriptThread(std::function<void()>&& func)
    {
        auto& engine = Detail::Cef::EngineState::instance();
        if (engine.onUiThread())
        {
            func();
            return;
        }
        engine.postToUi(gated(impl_->shared, std::move(func)));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::run()
    {
        Detail::Cef::EngineState::instance().runLoop();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::terminate()
    {
        auto& engine = Detail::Cef::EngineState::instance();
        if (engine.onUiThread())
        {
            impl_->closeFromUi();
            return;
        }
        engine.postToUi([weak = std::weak_ptr<Implementation>{impl_}]() {
            if (const auto self = weak.lock())
                self->closeFromUi();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback)
    {
        if (!callback)
            throw std::runtime_error("Callback must be valid.");
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->callbacks[name] = callback;
        }
        impl_->addScript(Detail::bindScript(name));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::unbind(std::string const& name)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->callbacks.erase(name);
        }
        impl_->addScript(Detail::unbindScript(name));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::eval(std::filesystem::path const& file)
    {
        const auto script = loadFile(file);
        if (!script)
            throw std::runtime_error("Could not load file: " + file.string());
        eval(*script);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::eval(char const* js)
    {
        eval(std::string{js});
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::eval(std::string const& js)
    {
        impl_->send([&js]() {
            return Detail::Cef::makeTextMessage(Detail::Cef::evalMessageName, js);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::callFrontend(std::string const& name, std::string const& jsonArgument)
    {
        impl_->send([&name, &jsonArgument]() {
            return Detail::Cef::makeCallMessage(name, jsonArgument);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::init(std::string const& js)
    {
        impl_->addScript(js);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::init(std::filesystem::path const& file)
    {
        const auto script = loadFile(file);
        if (!script)
            throw std::runtime_error("Could not load file: " + file.string());
        init(*script);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::openDevTools()
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (!impl_->debug)
                return;
        }
        Detail::Cef::EngineState::instance().postToUi([weak = std::weak_ptr<Implementation>{impl_}]() {
            const auto self = weak.lock();
            if (!self)
                return;
            CefRefPtr<CefBrowser> browser;
            {
                std::scoped_lock lock{self->mutex};
                browser = self->browser;
            }
            if (browser)
                browser->GetHost()->ShowDevTools(CefWindowInfo{}, nullptr, CefBrowserSettings{}, CefPoint{});
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setVirtualHostNameToFolderMapping(
        std::string const& hostName,
        std::filesystem::path const& folderPath,
        HostResourceAccessKind)
    {
        std::unique_lock lock{impl_->shared->folderMappingGuard};
        impl_->shared->folderMappings[hostName] = folderPath;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setConsoleOutput(bool active)
    {
        std::scoped_lock lock{impl_->mutex};
        impl_->showConsoleOutput = active;
    }
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::any_io_executor Window::getExecutor() const
    {
        return impl_->pool.executor();
    }
}
// #####################################################################################################################
namespace Nui::Cef
{
    void setClientHooks(Window& window, ClientHooks hooks)
    {
        auto& implementation = Detail::WindowAccess::of(window);
        std::scoped_lock lock{implementation.mutex};
        if (implementation.state != Window::Implementation::BrowserState::NotRequested)
            throw std::logic_error("Nui: client hooks must be set before the window's browser is created");
        implementation.hooks = std::move(hooks);
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefBrowser> browser(Window& window)
    {
        auto& implementation = Detail::WindowAccess::of(window);
        std::scoped_lock lock{implementation.mutex};
        return implementation.browser;
    }
    //---------------------------------------------------------------------------------------------------------------------
    NativeWindowHandle nativeWindowHandle(Window& window)
    {
        auto& implementation = Detail::WindowAccess::of(window);
        CefRefPtr<CefWindow> cefWindow;
        {
            std::scoped_lock lock{implementation.mutex};
            cefWindow = implementation.cefWindow;
        }
        if (!cefWindow)
            return {};
        const auto handle = cefWindow->GetWindowHandle();
#ifdef __APPLE__
        return {.kind = NativeWindowHandle::Kind::MacWindow, .value = Detail::Cef::windowOfView(handle)};
#else
        auto platform = CefCommandLine::GetGlobalCommandLine()->GetSwitchValue("ozone-platform").ToString();
        if (platform.empty())
        {
            auto const* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
            platform = waylandDisplay != nullptr && *waylandDisplay != '\0' ? "wayland" : "x11";
        }
        return {
            .kind = platform == "x11" ? NativeWindowHandle::Kind::X11 : NativeWindowHandle::Kind::Wayland,
            .value = static_cast<std::uintptr_t>(handle),
        };
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string portalParentWindow(Window& window)
    {
        const auto handle = nativeWindowHandle(window);
        if (handle.kind != NativeWindowHandle::Kind::X11)
            return {};
        return fmt::format("x11:{:x}", handle.value);
    }
}
