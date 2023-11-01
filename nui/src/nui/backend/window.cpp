#include <nui/window.hpp>

#include <nui/backend/filesystem/special_paths.hpp>
#include <nui/backend/filesystem/file_dialog.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/utility/widen.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/data_structures/selectables_registry.hpp>
#include <nui/screen.hpp>

#include <webview.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <roar/mime_type.hpp>
#include <roar/utility/scope_exit.hpp>

#if defined(__APPLE__)
#    include <objc/objc-runtime.h>
#    include <objc/NSObjCRuntime.h>
#    include <CoreGraphics/CoreGraphics.h>
#elif __linux__
#    include <gtk/gtk.h>
#    include <libsoup/soup.h>
#elif defined(_WIN32)
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

#ifdef _WIN32
#    include <webview2_environment_options.hpp>
#    include <webview2_iids.h>
#    include <wrl/event.h>
#endif

#if __linux__
struct HostNameMappingInfo
{
    std::unordered_map<std::string, std::filesystem::path> hostNameToFolderMapping{};
    std::size_t hostNameMappingMax{0};
};
#endif

#if defined(_WIN32)
constexpr static auto wakeUpMessage = WM_APP + 1;
#endif

namespace Nui
{
    namespace
    {
        static std::string loadFile(std::filesystem::path const& file)
        {
            std::ifstream reader{file, std::ios::binary};
            if (!reader)
                throw std::runtime_error("Could not open file: " + file.string());
            reader.seekg(0, std::ios::end);
            std::string content(static_cast<std::size_t>(reader.tellg()), '\0');
            reader.seekg(0, std::ios::beg);
            reader.read(content.data(), static_cast<std::streamsize>(content.size()));
            return content;
        }
    }

    // #####################################################################################################################
#ifdef _WIN32
    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions>
    webView2EnvironmentOptionsFromOptions(WindowOptions const& options)
    {
        auto environmentOptions = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();

        if (options.browserArguments)
        {
            const auto wideArgs = widenString(*options.browserArguments);
            environmentOptions->put_AdditionalBrowserArguments(wideArgs.c_str());
        }

        if (options.language)
        {
            const auto wideLanguage = widenString(*options.language);
            environmentOptions->put_Language(wideLanguage.c_str());
        }

        Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options4;
        if (environmentOptions.As(&options4) == S_OK)
        {
            std::vector<Microsoft::WRL::ComPtr<CoreWebView2CustomSchemeRegistration>> customSchemeRegistrations;
            std::vector<std::vector<std::wstring>> allowedOrigins;
            std::vector<std::vector<std::wstring::value_type const*>> allowedOriginsRaw;
            std::vector<std::wstring> wideSchemes;

            allowedOrigins.reserve(options.customSchemes.size());
            allowedOriginsRaw.reserve(options.customSchemes.size());
            wideSchemes.reserve(options.customSchemes.size());

            for (const auto& customScheme : options.customSchemes)
            {
                wideSchemes.push_back(widenString(customScheme.scheme));
                customSchemeRegistrations.push_back(
                    Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(wideSchemes.back().c_str()));
                auto& customSchemeRegistration = customSchemeRegistrations.back();

                allowedOrigins.push_back({});
                allowedOrigins.back().reserve(customScheme.allowedOrigins.size());
                for (const auto& allowedOrigin : customScheme.allowedOrigins)
                    allowedOrigins.back().push_back(widenString(allowedOrigin));

                allowedOriginsRaw.push_back({});
                allowedOriginsRaw.back().reserve(allowedOrigins.back().size());
                for (const auto& allowedOrigin : allowedOrigins.back())
                    allowedOriginsRaw.back().push_back(allowedOrigin.c_str());

                customSchemeRegistration->SetAllowedOrigins(
                    static_cast<UINT>(allowedOriginsRaw.back().size()), allowedOriginsRaw.back().data());
                customSchemeRegistration->put_TreatAsSecure(customScheme.treatAsSecure);
                customSchemeRegistration->put_HasAuthorityComponent(customScheme.hasAuthorityComponent);
            }
            std::vector<ICoreWebView2CustomSchemeRegistration*> customSchemeRegistrationsRaw;
            customSchemeRegistrationsRaw.reserve(customSchemeRegistrations.size());
            for (const auto& customSchemeRegistration : customSchemeRegistrations)
                customSchemeRegistrationsRaw.push_back(customSchemeRegistration.Get());

            const auto result = options4->SetCustomSchemeRegistrations(
                static_cast<UINT>(customSchemeRegistrationsRaw.size()), customSchemeRegistrationsRaw.data());
            if (FAILED(result))
                throw std::runtime_error("Could not set custom scheme registrations.");
        }

        Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions5> options5;
        if (environmentOptions.As(&options5) == S_OK)
        {
            options5->put_EnableTrackingPrevention(options.enableTrackingPrevention);
        }

        return environmentOptions;
    }
#endif
    // #####################################################################################################################
    struct Window::Implementation
    {
        webview::webview view;
        std::vector<std::filesystem::path> cleanupFiles;
        std::unordered_map<std::string, std::function<void(nlohmann::json const&)>> callbacks;
        boost::asio::thread_pool pool;
        std::recursive_mutex viewGuard;
        int width;
        int height;

        Implementation(WindowOptions const& options)
            : view{[&options]() -> webview::webview {
#ifdef _WIN32
                return {options.debug, nullptr, webView2EnvironmentOptionsFromOptions(options).Get()};
#endif
                return {options.debug, nullptr, nullptr};
            }()}
            , cleanupFiles{}
            , callbacks{}
            , pool{4}
            , viewGuard{}
            , width{0}
            , height{0}
        {}
        ~Implementation()
        {
            pool.stop();
            pool.join();
        }
    };

#ifdef __APPLE__
    // #####################################################################################################################
    struct Window::MacOsImplementation : public Window::Implementation
    {
        MacOsImplementation(WindowOptions const& options)
            : Implementation{options}
        {}
    };
    // #####################################################################################################################
#elif defined(__linux__)
    // #####################################################################################################################
    struct Window::LinuxImplementation : public Window::Implementation
    {
        struct SchemeContext
        {
            std::size_t id;
            std::weak_ptr<Window::LinuxImplementation> impl;

            std::string mime;
            GInputStream* stream;
            SoupMessageHeaders* headers;
            WebKitURISchemeResponse* response;
        };

        HostNameMappingInfo hostNameMappingInfo;
        std::recursive_mutex schemeResponseRegistryGuard;
        SelectablesRegistry<std::unique_ptr<Window::LinuxImplementation::SchemeContext>> schemeResponseRegistry;
        std::list<std::string> schemes{};

        LinuxImplementation(WindowOptions const& options)
            : Implementation{options}
            , hostNameMappingInfo{}
            , schemeResponseRegistryGuard{}
            , schemeResponseRegistry{}
            , schemes{}
        {}
    };
    // #####################################################################################################################
#endif

#ifdef _WIN32
    // #####################################################################################################################
    struct Window::WindowsImplementation : public Window::Implementation
    {
        DWORD windowThreadId;
        std::vector<std::function<void()>> toProcessOnWindowThread;
        EventRegistrationToken schemeHandlerToken;
        std::optional<EventRegistrationToken> setHtmlWorkaroundToken;

        WindowsImplementation(WindowOptions const& options)
            : Implementation{options}
            , windowThreadId{GetCurrentThreadId()}
            , toProcessOnWindowThread{}
            , schemeHandlerToken{}
            , setHtmlWorkaroundToken{}
        {
            registerSchemeHandlers(options);
        }

        void registerSchemeHandlers(WindowOptions const& options);
        HRESULT onSchemeRequest(
            std::vector<CustomScheme> const& schemes,
            ICoreWebView2*,
            ICoreWebView2WebResourceRequestedEventArgs* args);
        CustomSchemeRequest makeCustomSchemeRequest(
            CustomScheme const& scheme,
            std::string const& uri,
            COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext,
            ICoreWebView2WebResourceRequest* webViewRequest);
        Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse>
        makeResponse(CustomSchemeResponse const& responseData, HRESULT& result);
    };
    //---------------------------------------------------------------------------------------------------------------------
    void Window::WindowsImplementation::registerSchemeHandlers(WindowOptions const& options)
    {
        auto* webView = static_cast<ICoreWebView2*>(static_cast<webview::browser_engine&>(view).webview());

        for (auto const& customScheme : options.customSchemes)
        {
            const std::wstring filter = widenString(customScheme.scheme + ":*");

            auto result = webView->AddWebResourceRequestedFilter(
                filter.c_str(), static_cast<COREWEBVIEW2_WEB_RESOURCE_CONTEXT>(NuiCoreWebView2WebResourceContext::All));

            if (result != S_OK)
                throw std::runtime_error("Could not register custom scheme: " + customScheme.scheme);
            ;
        }

        webView->add_WebResourceRequested(
            Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [this, schemes = options.customSchemes](
                    ICoreWebView2* view, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                    return onSchemeRequest(schemes, view, args);
                })
                .Get(),
            &schemeHandlerToken);
    }
    //---------------------------------------------------------------------------------------------------------------------
    HRESULT
    Window::WindowsImplementation::onSchemeRequest(
        std::vector<CustomScheme> const& schemes,
        ICoreWebView2*,
        ICoreWebView2WebResourceRequestedEventArgs* args)
    {
        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext;
        auto result = args->get_ResourceContext(&resourceContext);
        if (result != S_OK)
            return result;

        Microsoft::WRL::ComPtr<ICoreWebView2WebResourceRequest> webViewRequest;
        args->get_Request(&webViewRequest);

        const auto uri = [&webViewRequest]() {
            LPWSTR uri;
            webViewRequest->get_Uri(&uri);
            std::wstring uriW{uri};
            CoTaskMemFree(uri);
            return shortenString(uriW);
        }();

        const auto customScheme = [&schemes, &uri]() -> std::optional<CustomScheme> {
            // assuming short schemes list, a linear search is fine
            for (auto const& scheme : schemes)
            {
                if (uri.starts_with(scheme.scheme + ":"))
                    return scheme;
            }
            return std::nullopt;
        }();

        if (!customScheme)
            return E_NOTIMPL;

        if (!customScheme->onRequest)
            return E_NOTIMPL;

        CustomSchemeRequest request =
            makeCustomSchemeRequest(*customScheme, uri, resourceContext, webViewRequest.Get());
        auto response = makeResponse(customScheme->onRequest(request), result);

        if (result != S_OK)
            return result;

        result = args->put_Response(response.Get());
        return result;
    }
    //---------------------------------------------------------------------------------------------------------------------
    Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse>
    Window::WindowsImplementation::makeResponse(CustomSchemeResponse const& responseData, HRESULT& result)
    {
        auto* webView = static_cast<ICoreWebView2*>(static_cast<webview::browser_engine&>(view).webview());

        Microsoft::WRL::ComPtr<ICoreWebView2WebResourceResponse> response;
        Microsoft::WRL::ComPtr<ICoreWebView2_2> wv22;
        result = webView->QueryInterface(IID_PPV_ARGS(&wv22));

        Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment;
        wv22->get_Environment(&environment);

        if (result != S_OK)
            return {};

        std::wstring responseHeaders;
        for (auto const& [key, value] : responseData.headers)
            responseHeaders += widenString(key) + L": " + widenString(value) + L"\n";
        responseHeaders.pop_back();

        Microsoft::WRL::ComPtr<IStream> stream;
        stream.Attach(SHCreateMemStream(
            reinterpret_cast<const BYTE*>(responseData.body.data()), static_cast<UINT>(responseData.body.size())));

        const auto phrase = widenString(responseData.reasonPhrase);
        result = environment->CreateWebResourceResponse(
            stream.Get(), responseData.statusCode, phrase.c_str(), responseHeaders.c_str(), &response);

        return response;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CustomSchemeRequest Window::WindowsImplementation::makeCustomSchemeRequest(
        CustomScheme const& customScheme,
        std::string const& uri,
        COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext,
        ICoreWebView2WebResourceRequest* webViewRequest)
    {
        return CustomSchemeRequest{
            .scheme = customScheme.scheme,
            .getContent = [webViewRequest, contentMemo = std::string{}]() mutable -> std::string const& {
                if (!contentMemo.empty())
                    return contentMemo;

                Microsoft::WRL::ComPtr<IStream> stream;
                webViewRequest->get_Content(&stream);

                if (!stream)
                    return contentMemo;

                // FIXME: Dont read the whole thing into memory, if possible via streaming.
                ULONG bytesRead = 0;
                do
                {
                    std::array<char, 1024> buffer;
                    stream->Read(buffer.data(), 1024, &bytesRead);
                    contentMemo.append(buffer.data(), bytesRead);
                } while (bytesRead == 1024);
                return contentMemo;
            },
            .headers =
                [webViewRequest]() {
                    ICoreWebView2HttpRequestHeaders* headers;
                    webViewRequest->get_Headers(&headers);

                    Microsoft::WRL::ComPtr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
                    headers->GetIterator(&iterator);

                    std::unordered_multimap<std::string, std::string> headersMap;
                    for (BOOL hasCurrent; SUCCEEDED(iterator->get_HasCurrentHeader(&hasCurrent)) && hasCurrent;)
                    {
                        LPWSTR name;
                        LPWSTR value;
                        iterator->GetCurrentHeader(&name, &value);
                        std::wstring nameW{name};
                        std::wstring valueW{value};
                        CoTaskMemFree(name);
                        CoTaskMemFree(value);

                        headersMap.emplace(shortenString(nameW), shortenString(valueW));

                        BOOL hasNext = FALSE;
                        if (FAILED(iterator->MoveNext(&hasNext)) || !hasNext)
                            break;
                    }
                    return headersMap;
                }(),
            .uri = uri,
            .method =
                [webViewRequest]() {
                    LPWSTR method;
                    webViewRequest->get_Method(&method);
                    std::wstring methodW{method};
                    CoTaskMemFree(method);
                    return shortenString(methodW);
                }(),
            .resourceContext = static_cast<NuiCoreWebView2WebResourceContext>(resourceContext),
        };
    }
    // #####################################################################################################################
#endif
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
    void uriSchemeRequestCallback(WebKitURISchemeRequest* request, gpointer userData)
    {
        auto* schemeContext = static_cast<Nui::Window::LinuxImplementation::SchemeContext*>(userData);

        const auto path = std::string_view{webkit_uri_scheme_request_get_path(request)};
        const auto uri = std::string_view{webkit_uri_scheme_request_get_uri(request)};
        const auto scheme = std::string_view{webkit_uri_scheme_request_get_scheme(request)};

        auto exitError = Nui::ScopeExit{[&] {
            auto* error =
                g_error_new(WEBKIT_DOWNLOAD_ERROR_DESTINATION, 1, "Invalid custom scheme / Host name mapping.");
            auto freeError = Nui::ScopeExit{[error] {
                g_error_free(error);
            }};
            webkit_uri_scheme_request_finish_error(request, error);
        }};

        auto impl = schemeContext->impl.lock();
        if (!impl)
            return;

        auto hostName = std::string{uri.data() + scheme.size() + 3, uri.size() - scheme.size() - 3 - path.size()};
        auto it = impl->hostNameMappingInfo.hostNameToFolderMapping.find(hostName);
        if (it == impl->hostNameMappingInfo.hostNameToFolderMapping.end())
        {
            std::cerr << "Host name mapping not found: " << hostName << "\n";
            return;
        }

        const auto filePath = it->second / std::string{path.data() + 1, path.size() - 1};
        auto fileContent = std::string{};
        {
            auto file = std::ifstream{filePath, std::ios::binary};
            if (!file)
            {
                std::cerr << "File not found: " << filePath << "\n";
                return;
            }
            file.seekg(0, std::ios::end);
            fileContent.resize(static_cast<std::size_t>(file.tellg()));
            file.seekg(0, std::ios::beg);
            file.read(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
        }

        exitError.disarm();

        schemeContext->stream =
            g_memory_input_stream_new_from_data(fileContent.c_str(), static_cast<gssize>(fileContent.size()), nullptr);
        auto deleteStream = Roar::ScopeExit{[schemeContext] {
            g_object_unref(schemeContext->stream);
        }};

        const auto maybeMime = Roar::extensionToMime(filePath.extension().string());
        schemeContext->mime = maybeMime ? *maybeMime : "application/octet-stream";

        schemeContext->response =
            webkit_uri_scheme_response_new(schemeContext->stream, static_cast<gint64>(fileContent.size()));
        webkit_uri_scheme_response_set_content_type(schemeContext->response, schemeContext->mime.c_str());
        webkit_uri_scheme_response_set_status(schemeContext->response, fileContent.size() > 0 ? 200 : 204, nullptr);

        schemeContext->headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
        soup_message_headers_append(schemeContext->headers, "Access-Control-Allow-Origin", "*");

        webkit_uri_scheme_response_set_http_headers(schemeContext->response, schemeContext->headers);
        webkit_uri_scheme_request_finish_with_response(request, schemeContext->response);
    }

    void uriSchemeDestroyNotify(void*)
    {
        // Happens when everything else is already dead.
    }
}
#endif

namespace Nui
{
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
        : impl_{std::make_shared<MacOsImplementation>(options)}
#elif defined(__linux__)
        : impl_{std::make_shared<LinuxImplementation>(options)}
#elif defined(_WIN32)
        : impl_{std::make_shared<WindowsImplementation>(options)}
#endif
    {
        impl_->view.install_message_hook([this](std::string const& msg) {
            std::scoped_lock lock{impl_->viewGuard};
            const auto obj = nlohmann::json::parse(msg);
            impl_->callbacks[obj["id"].get<std::string>()](obj["args"]);
            return false;
        });
        // TODO: SetCustomSchemeRegistrations

        if (options.title)
            setTitle(*options.title);

#ifdef __APPLE__
#elif defined(__linux__)
        auto* linuxImpl = static_cast<LinuxImplementation*>(impl_.get());
        std::lock_guard schemeResponseRegistryLock{linuxImpl->schemeResponseRegistryGuard};
        const auto id =
            linuxImpl->schemeResponseRegistry.emplace(std::make_unique<Window::LinuxImplementation::SchemeContext>());
        auto& entry = linuxImpl->schemeResponseRegistry[id];
        entry->id = id;
        entry->impl = std::static_pointer_cast<Window::LinuxImplementation>(impl_);

        linuxImpl->schemes.push_back(options.localScheme);

        auto nativeWebView = WEBKIT_WEB_VIEW(getNativeWebView());
        auto* webContext = webkit_web_view_get_context(nativeWebView);
        webkit_web_context_register_uri_scheme(
            webContext,
            linuxImpl->schemes.back().c_str(),
            &uriSchemeRequestCallback,
            entry.get(),
            &uriSchemeDestroyNotify);
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
        impl_->view.set_title(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->width = width;
        impl_->height = height;
        impl_->view.set_size(width, height, static_cast<int>(hint));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setHtml(std::string_view html, bool fromFilesystem, bool windowsForceNoFilesystem)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        // https://github.com/MicrosoftEdge/WebView2Feedback/issues/1355
        // :((((
        if (!windowsForceNoFilesystem)
            fromFilesystem = true;
#endif
        if (fromFilesystem)
        {
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
        }
        else
        {
#ifdef _WIN32
            if (html.size() < 1'900'000)
            {
                impl_->view.set_html(std::string{html});
                return;
            }
            else
            {
                throw std::runtime_error("HTML size too large for WebView2.");
            }
#else
            impl_->view.set_html(std::string{html});
#endif
        }
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
        impl_->view.navigate(url);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::filesystem::path& file)
    {
        using namespace std::string_literals;
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.navigate("file://"s + file.string());
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
                auto f = reinterpret_cast<std::function<void()>*>(msg.lParam);
                ScopeExit se{[f]() {
                    // yuck! but this is from webview internals
                    delete f;
                }};
                (*f)();
            }
            else if (msg.message == WM_QUIT)
                return;
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
    void Window::setPosition(int x, int y, bool useFrameOrigin)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        (void)useFrameOrigin;
        SetWindowPos(reinterpret_cast<HWND>(impl_->view.window()), nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
#elif defined(__APPLE__)
        using namespace webview::detail;
        auto wnd = static_cast<id>(impl_->view.window());
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
        gtk_window_move(static_cast<GtkWindow*>(impl_->view.window()), x, y);
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

            impl_->view.init(script);
            impl_->view.eval(script);
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
            impl_->view.init(script);
            impl_->view.eval(script);
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
        impl_->view.eval(loadFile(file));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::init(std::string const& js)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.init(js);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::init(std::filesystem::path const& file)
    {
        std::scoped_lock lock{impl_->viewGuard};
        impl_->view.init(loadFile(file));
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
            winImpl->view.eval(js);
        }
        else
        {
            winImpl->toProcessOnWindowThread.push_back([js, winImpl]() {
                winImpl->view.eval(js);
            });
            PostThreadMessage(winImpl->windowThreadId, wakeUpMessage, 0, 0);
        }
#elif defined(__APPLE__)
        impl_->view.eval(js);
#else
        impl_->view.eval(js);
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void* Window::getNativeWebView()
    {
        return static_cast<webview::browser_engine&>(impl_->view).webview();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void* Window::getNativeWindow()
    {
        return impl_->view.window();
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
        auto releaseInterface = Roar::ScopeExit{[wv23] {
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
        throw std::runtime_error("Not implemented");
        (void)hostName;
        (void)folderPath;
        (void)accessKind;
        // // setURLSchemeHandler
        // Protocol* proto = objc_allocateProtocol("WKURLSchemeHandler");

        // protocol_addMethodDescription(
        //     proto,
        //     sel_registerName("webView:startURLSchemeTask:"),
        //     "v@:@@", // return type, self, selector, first argument
        //     true,
        //     true); // is required
        // protocol_addMethodDescription(
        //     proto,
        //     sel_registerName("webView:stopURLSchemeTask:"),
        //     "v@:@@", // return type, self, selector, first argument
        //     true,
        //     true); // is required

        // objc_registerProtocol(proto);
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