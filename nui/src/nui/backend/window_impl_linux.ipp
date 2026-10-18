#include "gobject.hpp"

namespace Nui::Impl::Linux
{
    using ReaderFn = std::function<std::size_t(char*, std::size_t)>;

    // GInputStream subclass that pulls bytes from a std::function reader.
    // GType demands a POD layout so the ReaderFn lives behind a pointer; ownership is passed via
    // unique_ptr in nuiReaderInputStreamNew and re-adopted by unique_ptr in the finalize hook.
    struct NuiReaderInputStream
    {
        GInputStream parent;
        ReaderFn* reader;
    };
    struct NuiReaderInputStreamClass
    {
        GInputStreamClass parent_class;
    };

    inline gssize nuiReaderInputStreamRead(
        GInputStream* stream, void* buffer, gsize count, GCancellable* /*cancellable*/, GError** error)
    {
        auto* self = reinterpret_cast<NuiReaderInputStream*>(stream);
        if (self->reader == nullptr)
            return 0;
        try
        {
            return static_cast<gssize>((*self->reader)(static_cast<char*>(buffer), count));
        }
        catch (std::exception const& ex)
        {
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, ex.what());
            return -1;
        }
        catch (...)
        {
            g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_FAILED, "bodyReader threw a non-std::exception");
            return -1;
        }
    }
    inline gboolean nuiReaderInputStreamClose(GInputStream* /*stream*/, GCancellable* /*cancellable*/, GError** /*error*/)
    {
        return TRUE;
    }
    inline void nuiReaderInputStreamFinalize(GObject* obj)
    {
        auto* self = reinterpret_cast<NuiReaderInputStream*>(obj);
        std::unique_ptr<ReaderFn> owned{self->reader};
        self->reader = nullptr;
        G_OBJECT_CLASS(g_type_class_peek_parent(G_OBJECT_GET_CLASS(obj)))->finalize(obj);
    }
    inline void nuiReaderInputStreamClassInit(gpointer klass, gpointer /*classData*/)
    {
        G_OBJECT_CLASS(klass)->finalize = nuiReaderInputStreamFinalize;
        G_INPUT_STREAM_CLASS(klass)->read_fn = nuiReaderInputStreamRead;
        G_INPUT_STREAM_CLASS(klass)->close_fn = nuiReaderInputStreamClose;
    }
    inline void nuiReaderInputStreamInit(GTypeInstance* instance, gpointer /*klass*/)
    {
        reinterpret_cast<NuiReaderInputStream*>(instance)->reader = nullptr;
    }
    inline GType nuiReaderInputStreamGetType()
    {
        static GType type = []() {
            GTypeInfo info{};
            info.class_size = sizeof(NuiReaderInputStreamClass);
            info.class_init = nuiReaderInputStreamClassInit;
            info.instance_size = sizeof(NuiReaderInputStream);
            info.instance_init = nuiReaderInputStreamInit;
            return g_type_register_static(
                G_TYPE_INPUT_STREAM, "NuiReaderInputStream", &info, static_cast<GTypeFlags>(0));
        }();
        return type;
    }
    inline GInputStream* nuiReaderInputStreamNew(ReaderFn reader)
    {
        auto* self = static_cast<NuiReaderInputStream*>(g_object_new(nuiReaderInputStreamGetType(), nullptr));
        self->reader = std::make_unique<ReaderFn>(std::move(reader)).release();
        return reinterpret_cast<GInputStream*>(self);
    }

    struct AsyncResponse
    {
        GObjectReference<GInputStream> stream;
        GObjectReference<WebKitURISchemeResponse> response;
        std::string data;
    };

    struct SchemeContext
    {
        CustomScheme schemeInfo;
        std::mutex asyncResponsesGuard;
        std::map<int, AsyncResponse> asyncResponses;
        int asyncResponseCounter = 0;

        void gcResponses()
        {
            std::lock_guard<std::mutex> lock{this->asyncResponsesGuard};
            std::vector<int> removals{};
            for (auto it = asyncResponses.begin(); it != asyncResponses.end(); ++it)
            {
                GInputStream* stream = it->second.stream.get();
                if (g_input_stream_is_closed(stream))
                {
                    removals.push_back(it->first);
                    break;
                }
            }
            for (auto const& removal : removals)
                asyncResponses.erase(removal);
        }
    };
}

std::size_t strlenLimited(char const* str, std::size_t limit)
{
    std::size_t i = 0;
    while (i <= limit && str[i] != '\0')
        ++i;
    return i;
}

namespace Nui::Impl::Linux
{
    /**
     * @brief Fails a custom scheme request, the page sees a network error.
     */
    inline void finishWithError(WebKitURISchemeRequest* request, char const* message)
    {
        std::unique_ptr<GError, decltype(&g_error_free)> error{
            g_error_new(WEBKIT_DOWNLOAD_ERROR_DESTINATION, 1, "%s", message), &g_error_free};
        webkit_uri_scheme_request_finish_error(request, error.get());
    }

    /**
     * @brief Serves a custom scheme request with the handler of the window that owns the requesting web view.
     */
    inline void dispatchSchemeRequest(WebKitURISchemeRequest* request, std::string const& scheme);
}

extern "C" {
    void uriSchemeRequestCallback(WebKitURISchemeRequest* request, gpointer userData)
    {
        // Exceptions must not cross the C boundary into glib/webkit. Convert any escape into a
        // synthetic finish_error so the renderer is unblocked.
        try
        {
            Nui::Impl::Linux::dispatchSchemeRequest(request, *static_cast<std::string const*>(userData));
        }
        catch (std::exception const& ex)
        {
            Nui::Impl::Linux::finishWithError(request, ex.what());
        }
        catch (...)
        {
            Nui::Impl::Linux::finishWithError(request, "Unknown C++ exception in custom scheme handler");
        }
    }
}

namespace Nui::Impl::Linux
{
    /**
     * @brief Answers a custom scheme request with the handler of a scheme context. The caller keeps the context alive.
     */
    inline void serveSchemeRequest(WebKitURISchemeRequest* request, SchemeContext* schemeContext)
    {
        using namespace std::string_literals;

        schemeContext->gcResponses();

        // const auto path = std::string_view{webkit_uri_scheme_request_get_path(request)};
        // const auto scheme = std::string_view{webkit_uri_scheme_request_get_scheme(request)};
        const auto uri = std::string_view{webkit_uri_scheme_request_get_uri(request)};

        char const* cmethod = webkit_uri_scheme_request_get_http_method(request);
        if (cmethod == nullptr)
            cmethod = "";

        auto const& schemeInfo = schemeContext->schemeInfo;
        const bool streaming = schemeInfo.streamingContent;

        // webkit_uri_scheme_request_get_http_body returns transfer-full; GObjectReference handles the
        // unref via RAII. close() is an I/O flush, not memory management, so it stays explicit.
        struct LinuxStreamState
        {
            Nui::GObjectReference<GInputStream> stream{};
            bool initialized = false;
            ~LinuxStreamState()
            {
                if (stream)
                    g_input_stream_close(stream.get(), nullptr, nullptr);
            }
        };
        auto streamState = std::make_shared<LinuxStreamState>();

        // Hold a ref on the request so the body-reading lambdas stay safe even if a user copies them
        // out of the synchronous onRequest scope (the API allows it; we don't want UAF when they do).
        auto requestRef = Nui::GObjectReference<WebKitURISchemeRequest>{request};

        const auto responseObj = schemeInfo.onRequest(
            Nui::CustomSchemeRequest{
                .scheme = schemeInfo.scheme,
                .getContent = streaming ? std::function<std::string()>{} : std::function<std::string()>{[requestRef]() -> std::string {
#if (WEBKIT_MAJOR_VERSION == 2 && WEBKIT_MINOR_VERSION >= 40) || WEBKIT_MAJOR_VERSION > 2
                    auto stream = Nui::GObjectReference<GInputStream>::adoptReference(
                        webkit_uri_scheme_request_get_http_body(requestRef.get()));
                    if (!stream)
                        return std::string{};
                    Nui::ScopeExit closeStream = Nui::ScopeExit{[s = stream.get()]() noexcept {
                        g_input_stream_close(s, nullptr, nullptr);
                    }};

                    auto dataInputStream = Nui::GObjectReference<GDataInputStream>::adoptReference(
                        g_data_input_stream_new(stream.get()));
                    gsize length = 0;
                    GError* errorRaw = nullptr;
                    gchar* dataRaw = g_data_input_stream_read_upto(dataInputStream.get(), "", 0, &length, nullptr, &errorRaw);
                    std::unique_ptr<gchar, decltype(&g_free)> data{dataRaw, &g_free};
                    std::unique_ptr<GError, decltype(&g_error_free)> error{errorRaw, &g_error_free};

                    if (error)
                        return {};
                    if (!data)
                        return {};
                    return std::string(data.get(), length);
#else
                    // Not implemented in earlier webkitgtk versions :(
                    return std::string{};
#endif
                }},
                .readContent = streaming
                    ? std::function<std::size_t(char*, std::size_t)>{[requestRef, streamState](char* buffer, std::size_t bufferSize) -> std::size_t {
#if (WEBKIT_MAJOR_VERSION == 2 && WEBKIT_MINOR_VERSION >= 40) || WEBKIT_MAJOR_VERSION > 2
                          if (!streamState->initialized)
                          {
                              streamState->stream = Nui::GObjectReference<GInputStream>::adoptReference(
                                  webkit_uri_scheme_request_get_http_body(requestRef.get()));
                              streamState->initialized = true;
                          }
                          if (!streamState->stream)
                              return 0;
                          GError* errorRaw = nullptr;
                          const gssize bytesRead =
                              g_input_stream_read(streamState->stream.get(), buffer, bufferSize, nullptr, &errorRaw);
                          std::unique_ptr<GError, decltype(&g_error_free)> error{errorRaw, &g_error_free};
                          if (error || bytesRead <= 0)
                              return 0;
                          return static_cast<std::size_t>(bytesRead);
#else
                          (void)requestRef;
                          (void)streamState;
                          (void)buffer;
                          (void)bufferSize;
                          return 0;
#endif
                      }}
                    : std::function<std::size_t(char*, std::size_t)>{},
                .headers =
                    [request]() {
                        auto* headers = webkit_uri_scheme_request_get_http_headers(request);
                        auto headersMap = std::unordered_multimap<std::string, std::string>{};

                        SoupMessageHeadersIter iter;
                        const char *name, *value;

                        soup_message_headers_iter_init(&iter, headers);
                        while (soup_message_headers_iter_next(&iter, &name, &value))
                        {
                            headersMap.insert({name, value});
                        }

                        return headersMap;
                    }(),
                .uri = std::string{uri},
                .method = std::string{cmethod},
            });

        using Nui::GObjectReference;

        std::lock_guard<std::mutex> asyncResponsesGuard{schemeContext->asyncResponsesGuard};
        ++schemeContext->asyncResponseCounter;
        schemeContext->asyncResponses[schemeContext->asyncResponseCounter] = Nui::Impl::Linux::AsyncResponse{};
        auto& asyncResponse = schemeContext->asyncResponses[schemeContext->asyncResponseCounter];

        // Move headers aside so we can inject a Content-Length for bodyFile without mutating the original.
        auto responseHeaders = std::move(responseObj.headers);
        gint64 contentLength = -1; // -1 = unknown (chunked); webkit_uri_scheme_response_new accepts this
        int statusCode = responseObj.statusCode;

        // Resolve body source in priority order: bodyFile > bodyReader > body.
        if (responseObj.bodyFile)
        {
            const auto& path = *responseObj.bodyFile;
            auto file = GObjectReference<GFile>::adoptReference(g_file_new_for_path(path.c_str()));
            GError* errorRaw = nullptr;
            GFileInputStream* fileStream = g_file_read(file.get(), nullptr, &errorRaw);
            std::unique_ptr<GError, decltype(&g_error_free)> error{errorRaw, &g_error_free};

            if (fileStream != nullptr)
            {
                asyncResponse.stream = GObjectReference<GInputStream>::adoptReference(G_INPUT_STREAM(fileStream));
                std::error_code ec;
                const auto size = std::filesystem::file_size(path, ec);
                if (!ec)
                {
                    contentLength = static_cast<gint64>(size);
                    if (responseHeaders.find("Content-Length") == responseHeaders.end())
                        responseHeaders.emplace("Content-Length", std::to_string(size));
                }
            }
            else
            {
                // Surface the failure as 500 instead of silently emitting an empty 200 — silent
                // success-with-empty-body is a cache-poisoning footgun.
                statusCode = 500;
                asyncResponse.data = "Internal Server Error: bodyFile could not be opened.";
                asyncResponse.stream =
                    GObjectReference<GInputStream>::adoptReference(g_memory_input_stream_new_from_data(
                        asyncResponse.data.data(), static_cast<gssize>(asyncResponse.data.size()), nullptr));
                contentLength = static_cast<gint64>(asyncResponse.data.size());
                responseHeaders.erase("Content-Length");
                responseHeaders.emplace("Content-Length", std::to_string(asyncResponse.data.size()));
                responseHeaders.erase("Content-Type");
                responseHeaders.emplace("Content-Type", "text/plain; charset=utf-8");
            }
        }
        else if (responseObj.bodyReader)
        {
            asyncResponse.stream = GObjectReference<GInputStream>::adoptReference(
                Nui::Impl::Linux::nuiReaderInputStreamNew(std::move(responseObj.bodyReader)));
            // length unknown → -1 (chunked)
        }
        else
        {
            asyncResponse.data = std::move(responseObj.body);
            asyncResponse.stream =
                GObjectReference<GInputStream>::adoptReference(g_memory_input_stream_new_from_data(
                    asyncResponse.data.data(), static_cast<gssize>(asyncResponse.data.size()), nullptr));
            contentLength = static_cast<gint64>(asyncResponse.data.size());
        }

        asyncResponse.response = GObjectReference<WebKitURISchemeResponse>::adoptReference(
            webkit_uri_scheme_response_new(asyncResponse.stream.get(), contentLength));

        const std::string contentType = [&]() {
            if (responseHeaders.find("Content-Type") != responseHeaders.end())
            {
                std::string contentType;
                auto range = responseHeaders.equal_range("Content-Type");
                for (auto it = range.first; it != range.second; ++it)
                    contentType += it->second + "; ";
                contentType.pop_back();
                contentType.pop_back();
                return contentType;
            }
            return "application/octet-stream"s;
        }();

        webkit_uri_scheme_response_set_content_type(asyncResponse.response.get(), contentType.c_str());
        webkit_uri_scheme_response_set_status(
            asyncResponse.response.get(), static_cast<guint>(statusCode), nullptr);

        auto setHeaders = [&]() {
            auto* responseHeadersObj = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
            for (auto const& [key, value] : responseHeaders)
                soup_message_headers_append(responseHeadersObj, key.c_str(), value.c_str());

            if (responseHeaders.find("Access-Control-Allow-Origin") == responseHeaders.end() &&
                !schemeInfo.allowedOrigins.empty())
            {
                auto const& front = schemeInfo.allowedOrigins.front();
                soup_message_headers_append(responseHeadersObj, "Access-Control-Allow-Origin", front.c_str());
            }
            webkit_uri_scheme_response_set_http_headers(asyncResponse.response.get(), responseHeadersObj);
        };

        setHeaders();
        webkit_uri_scheme_request_finish_with_response(request, asyncResponse.response.get());
    }
}

extern "C" {
    void uriSchemeDestroyNotify(void* userData)
    {
        // Called when the web context is finalized, takes back the scheme name given at the registration.
        std::unique_ptr<std::string> scheme{static_cast<std::string*>(userData)};
    }
}

namespace Nui
{
    // #####################################################################################################################
    struct Window::LinuxImplementation : public Window::Implementation
    {
        HostNameMappingInfo hostNameMappingInfo;
        std::mutex schemeContextsGuard;
        /// Custom schemes of this window by name.
        std::unordered_map<std::string, std::unique_ptr<Nui::Impl::Linux::SchemeContext>> schemeContexts;
        /// Set when the scheme handlers are registered, the view lives as long as the base implementation.
        WebKitWebView* webView;

        LinuxImplementation()
            : Implementation{}
            , hostNameMappingInfo{}
            , schemeContextsGuard{}
            , schemeContexts{}
            , webView{nullptr}
        {}
        ~LinuxImplementation() override;

        void registerSchemeHandlers(WindowOptions const& options) override;
        void registerSchemeHandler(CustomScheme const& scheme);

        /**
         * @brief The context of a custom scheme of this window, nullptr if the window has no scheme of that name.
         */
        Nui::Impl::Linux::SchemeContext* schemeContextOf(std::string const& scheme);
    };
    // #####################################################################################################################
}

namespace Nui::Impl::Linux
{
    /**
     * @brief The windows of the process by their web view.
     *
     * WebKitGTK takes one handler per scheme name and web context, and all windows share the default context. That
     * handler finds the window of the requesting web view here.
     */
    class WindowRegistry
    {
      public:
        static WindowRegistry& instance()
        {
            static WindowRegistry registry;
            return registry;
        }

        void add(WebKitWebView* webView, std::weak_ptr<Window::LinuxImplementation> window)
        {
            std::scoped_lock lock{guard_};
            windows_.insert_or_assign(webView, std::move(window));
        }

        void remove(WebKitWebView* webView)
        {
            std::scoped_lock lock{guard_};
            windows_.erase(webView);
        }

        /**
         * @brief The window of a web view, or for requests without a web view the only window with the scheme.
         */
        std::shared_ptr<Window::LinuxImplementation> windowOf(WebKitWebView* webView, std::string const& scheme) const
        {
            std::vector<std::weak_ptr<Window::LinuxImplementation>> windows;
            {
                std::scoped_lock lock{guard_};
                if (webView != nullptr)
                {
                    const auto iter = windows_.find(webView);
                    return iter == windows_.end() ? nullptr : iter->second.lock();
                }
                for (auto const& entry : windows_)
                    windows.push_back(entry.second);
            }
            // Outside the lock, a window released here may remove itself.
            std::shared_ptr<Window::LinuxImplementation> candidate;
            for (auto const& weak : windows)
            {
                auto window = weak.lock();
                if (!window || window->schemeContextOf(scheme) == nullptr)
                    continue;
                if (candidate)
                    return nullptr;
                candidate = std::move(window);
            }
            return candidate;
        }

      private:
        mutable std::mutex guard_{};
        std::unordered_map<WebKitWebView*, std::weak_ptr<Window::LinuxImplementation>> windows_{};
    };

    /**
     * @brief Registers the scheme on the web context unless a window did so before, WebKitGTK refuses a second one.
     */
    inline void registerSchemeOnContext(WebKitWebContext* webContext, std::string const& scheme)
    {
        const auto key = "nui-custom-scheme-" + scheme;
        if (g_object_get_data(G_OBJECT(webContext), key.c_str()) != nullptr)
            return;
        g_object_set_data(G_OBJECT(webContext), key.c_str(), GINT_TO_POINTER(1));
        webkit_web_context_register_uri_scheme(
            webContext,
            scheme.c_str(),
            &uriSchemeRequestCallback,
            std::make_unique<std::string>(scheme).release(),
            &uriSchemeDestroyNotify);
    }

    inline void dispatchSchemeRequest(WebKitURISchemeRequest* request, std::string const& scheme)
    {
        // Holds the window, and with it the scheme context, while the handler runs.
        const auto window =
            WindowRegistry::instance().windowOf(webkit_uri_scheme_request_get_web_view(request), scheme);
        if (!window)
        {
            finishWithError(request, "No window for this request");
            return;
        }
        auto* schemeContext = window->schemeContextOf(scheme);
        if (schemeContext == nullptr)
        {
            finishWithError(request, "Scheme not registered by this window");
            return;
        }
        serveSchemeRequest(request, schemeContext);
    }
}

namespace Nui
{
    // #####################################################################################################################
    Window::LinuxImplementation::~LinuxImplementation()
    {
        // The base still owns the view, so no other window can have its address yet.
        if (webView != nullptr)
            Nui::Impl::Linux::WindowRegistry::instance().remove(webView);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::LinuxImplementation::registerSchemeHandlers(WindowOptions const& options)
    {
        auto nativeWebView = static_cast<webview::browser_engine&>(*view).webview();
        if (!nativeWebView.has_value())
            throw std::runtime_error("Could not get native webview for registering custom scheme handler!");
        webView = WEBKIT_WEB_VIEW(nativeWebView.value());
        Nui::Impl::Linux::WindowRegistry::instance().add(webView, weak_from_base<LinuxImplementation>());

        for (auto const& scheme : options.customSchemes)
            registerSchemeHandler(scheme);

        if (options.folderMappingScheme)
            registerSchemeHandler(
                CustomScheme{
                    .scheme = *options.folderMappingScheme,
                    .allowedOrigins = {"*"},
                    .onRequest =
                        [weak = weak_from_base<Window::LinuxImplementation>()](CustomSchemeRequest const& req) {
                            auto shared = weak.lock();
                            if (!shared)
                                return CustomSchemeResponse{.statusCode = 500, .body = "Window is dead"};

                            return folderMappingResponseFromRequest(req, shared->hostNameMappingInfo);
                        },
                });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::LinuxImplementation::registerSchemeHandler(CustomScheme const& scheme)
    {
        {
            std::scoped_lock lock{schemeContextsGuard};
            auto& schemeContext = schemeContexts[scheme.scheme];
            // A later scheme of the same name is ignored.
            if (schemeContext)
                return;
            schemeContext = std::make_unique<Nui::Impl::Linux::SchemeContext>();
            schemeContext->schemeInfo = scheme;
        }
        Nui::Impl::Linux::registerSchemeOnContext(webkit_web_view_get_context(webView), scheme.scheme);
    }
    //---------------------------------------------------------------------------------------------------------------------
    Nui::Impl::Linux::SchemeContext* Window::LinuxImplementation::schemeContextOf(std::string const& scheme)
    {
        std::scoped_lock lock{schemeContextsGuard};
        const auto iter = schemeContexts.find(scheme);
        return iter == schemeContexts.end() ? nullptr : iter->second.get();
    }
    // #####################################################################################################################
}