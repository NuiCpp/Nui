namespace Nui::Impl::Linux
{
    struct SchemeContext
    {
        std::size_t id;
        std::weak_ptr<Window::LinuxImplementation> impl;
        CustomScheme schemeInfo;
    };
}

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
        auto* schemeContext = static_cast<Nui::Impl::Linux::SchemeContext*>(userData);

        // const auto path = std::string_view{webkit_uri_scheme_request_get_path(request)};
        // const auto scheme = std::string_view{webkit_uri_scheme_request_get_scheme(request)};
        const auto uri = std::string_view{webkit_uri_scheme_request_get_uri(request)};

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

        exitError.disarm();

        char const* cmethod = webkit_uri_scheme_request_get_http_method(request);
        if (cmethod == nullptr)
            cmethod = "";

        auto const& schemeInfo = schemeContext->schemeInfo;
        const auto responseObj = schemeInfo.onRequest(Nui::CustomSchemeRequest{
            .scheme = schemeInfo.scheme,
            .getContent = [request]() -> std::string {
                auto* stream = webkit_uri_scheme_request_get_http_body(request);
                if (stream == nullptr)
                    return std::string{};
                Nui::ScopeExit deleteStream = Nui::ScopeExit{[stream] {
                    g_input_stream_close(stream, nullptr, nullptr);
                }};

                // read the ginputstream to string
                GDataInputStream* dataInputStream = g_data_input_stream_new(stream);
                gsize length;
                GError* error = NULL;
                gchar* data = g_data_input_stream_read_upto(dataInputStream, "", 0, &length, NULL, &error);

                Nui::ScopeExit freeData = Nui::ScopeExit{[data] {
                    g_free(data);
                }};
                Nui::ScopeExit freeError = Nui::ScopeExit{[error] {
                    g_error_free(error);
                }};

                if (error != NULL)
                {
                    freeData.disarm();
                    return {};
                }

                freeError.disarm();
                return std::string(data, length);
            },
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

        GInputStream* stream;
        stream = g_memory_input_stream_new_from_data(
            responseObj.body.c_str(), static_cast<gssize>(responseObj.body.size()), nullptr);
        auto deleteStream = Nui::ScopeExit{[stream] {
            g_object_unref(stream);
        }};

        WebKitURISchemeResponse* response;
        response = webkit_uri_scheme_response_new(stream, static_cast<gint64>(responseObj.body.size()));

        std::string contentType;
        if (responseObj.headers.find("Content-Type") != responseObj.headers.end())
        {
            auto range = responseObj.headers.equal_range("Content-Type");
            for (auto it = range.first; it != range.second; ++it)
                contentType += it->second + "; ";
        }
        else
            contentType = "application/octet-stream";

        webkit_uri_scheme_response_set_content_type(response, contentType.c_str());
        webkit_uri_scheme_response_set_status(response, static_cast<guint>(responseObj.statusCode), nullptr);

        auto* responseHeaders = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
        auto deleteResponseHeaders = Nui::ScopeExit{[responseHeaders] {
            g_object_unref(responseHeaders);
        }};
        for (auto const& [key, value] : responseObj.headers)
            soup_message_headers_append(responseHeaders, key.c_str(), value.c_str());

        if (responseObj.headers.find("Access-Control-Allow-Origin") == responseObj.headers.end() &&
            !schemeInfo.allowedOrigins.empty())
        {
            auto const& front = schemeInfo.allowedOrigins.front();
            soup_message_headers_append(responseHeaders, "Access-Control-Allow-Origin", front.c_str());
        }

        webkit_uri_scheme_response_set_http_headers(response, responseHeaders);
        webkit_uri_scheme_request_finish_with_response(request, response);
    }

    void uriSchemeDestroyNotify(void*)
    {
        // Happens when everything else is already dead.
    }
}

namespace Nui
{
    // #####################################################################################################################
    struct Window::LinuxImplementation : public Window::Implementation
    {
        HostNameMappingInfo hostNameMappingInfo;
        std::recursive_mutex schemeResponseRegistryGuard;
        SelectablesRegistry<std::unique_ptr<Nui::Impl::Linux::SchemeContext>> schemeResponseRegistry;
        std::list<std::string> schemes{};

        LinuxImplementation()
            : Implementation{}
            , hostNameMappingInfo{}
            , schemeResponseRegistryGuard{}
            , schemeResponseRegistry{}
            , schemes{}
        {}

        void registerSchemeHandlers(WindowOptions const& options) override;
        void registerSchemeHandler(CustomScheme const& scheme);
    };
    //---------------------------------------------------------------------------------------------------------------------
    void Window::LinuxImplementation::registerSchemeHandlers(WindowOptions const& options)
    {
        for (auto const& scheme : options.customSchemes)
            registerSchemeHandler(scheme);

        if (options.folderMappingScheme)
            registerSchemeHandler(CustomScheme{
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
        std::lock_guard schemeResponseRegistryLock{schemeResponseRegistryGuard};
        const auto id = schemeResponseRegistry.emplace(std::make_unique<Nui::Impl::Linux::SchemeContext>());
        auto& entry = schemeResponseRegistry[id];
        entry->id = id;
        entry->impl = shared_from_base<LinuxImplementation>();
        entry->schemeInfo = scheme;

        schemes.push_back(scheme.scheme);
        auto nativeWebView = static_cast<webview::browser_engine&>(*view).webview();
        auto* webContext = webkit_web_view_get_context(WEBKIT_WEB_VIEW(nativeWebView));
        webkit_web_context_register_uri_scheme(
            webContext, schemes.back().c_str(), &uriSchemeRequestCallback, entry.get(), &uriSchemeDestroyNotify);
    }
    // #####################################################################################################################
}