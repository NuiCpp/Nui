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

#if __linux__
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
    struct Window::SchemeContext
    {
#ifdef __linux__
        std::size_t id;
        std::weak_ptr<Window::Implementation> impl;

        std::string mime;
        GInputStream* stream;
        SoupMessageHeaders* headers;
        WebKitURISchemeResponse* response;
#endif
    };
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
#if __linux__
        HostNameMappingInfo hostNameMappingInfo;
        std::recursive_mutex schemeResponseRegistryGuard;
        SelectablesRegistry<std::unique_ptr<Window::SchemeContext>> schemeResponseRegistry;
        std::list<std::string> schemes{};
#elif defined(_WIN32)
        DWORD windowThreadId;
        std::vector<std::function<void()>> toProcessOnWindowThread;
#endif

        Implementation(WindowOptions const& options)
            : view{[&options]() -> webview::webview {
#if __linux__
                return {options.debug, nullptr, nullptr};
#elif defined(_WIN32)
                // TODO: ICoreWebView2EnvironmentOptions
                return {options.debug, nullptr, nullptr};
#endif
            }()}
            , cleanupFiles{}
            , callbacks{}
            , pool{4}
            , viewGuard{}
            , width{0}
            , height{0}
#if __linux__
            , hostNameMappingInfo{}
            , schemeResponseRegistryGuard{}
            , schemeResponseRegistry{}
            , schemes{}
#elif defined(_WIN32)
            , windowThreadId{GetCurrentThreadId()}
            , toProcessOnWindowThread{}
#endif
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
    void uriSchemeRequestCallback(WebKitURISchemeRequest* request, gpointer userData)
    {
        auto* schemeContext = static_cast<Nui::Window::SchemeContext*>(userData);

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

    void uriSchemeDestroyNotify(void* data)
    {
        auto* schemeContext = static_cast<Nui::Window::SchemeContext*>(data);
        auto impl = schemeContext->impl.lock();
        if (!impl)
            return;

        std::lock_guard lock{impl->schemeResponseRegistryGuard};
        impl->schemeResponseRegistry.erase(schemeContext->id);
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
        : impl_{std::make_shared<Implementation>(options)}
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

#if __linux__
        std::lock_guard schemeResponseRegistryLock{impl_->schemeResponseRegistryGuard};
        const auto id = impl_->schemeResponseRegistry.emplace(std::make_unique<SchemeContext>());
        auto& entry = impl_->schemeResponseRegistry[id];
        entry.item.value()->id = id;
        entry.item.value()->impl = impl_;

        impl_->schemes.push_back(options.localScheme);

        auto nativeWebView = WEBKIT_WEB_VIEW(getNativeWebView());
        auto* webContext = webkit_web_view_get_context(nativeWebView);
        webkit_web_context_register_uri_scheme(
            webContext,
            impl_->schemes.back().c_str(),
            &uriSchemeRequestCallback,
            entry.item->get(),
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
    void Window::setHtml(std::string_view html, bool fromFilesystem)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        // https://github.com/MicrosoftEdge/WebView2Feedback/issues/1355
        // :((((
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
            impl_->view.set_html(std::string{html});
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
        MSG msg;
        BOOL res;
        while ((res = GetMessage(&msg, nullptr, 0, 0)) != -1)
        {
            {
                std::scoped_lock lock{impl_->viewGuard};
                if (!impl_->toProcessOnWindowThread.empty())
                {
                    for (auto const& func : impl_->toProcessOnWindowThread)
                        func();
                    impl_->toProcessOnWindowThread.clear();
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
    void Window::setPosition(int x, int y)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        SetWindowPos(reinterpret_cast<HWND>(impl_->view.window()), nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
#else
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
            primaryDisplay.y() + (primaryDisplay.height() - impl_->height) / 2);
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
        runInJavascriptThread([this, &name]() {
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
        if (GetCurrentThreadId() == impl_->windowThreadId)
            func();
        else
        {
            impl_->toProcessOnWindowThread.push_back(std::move(func));
            PostThreadMessage(impl_->windowThreadId, wakeUpMessage, 0, 0);
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
    void Window::eval(std::string const& js)
    {
        std::scoped_lock lock{impl_->viewGuard};
#if defined(_WIN32)
        if (GetCurrentThreadId() == impl_->windowThreadId)
        {
            impl_->view.eval(js);
        }
        else
        {
            impl_->toProcessOnWindowThread.push_back([this, js]() {
                impl_->view.eval(js);
            });
            PostThreadMessage(impl_->windowThreadId, wakeUpMessage, 0, 0);
        }
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