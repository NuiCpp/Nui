#pragma once

#include <nui/core.hpp>
#ifdef NUI_BACKEND
#    include <nlohmann/json.hpp>
#    include <boost/asio/any_io_executor.hpp>
#    include <nui/backend/url.hpp>
#    include <filesystem>
#endif

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <functional>
#include <unordered_map>

namespace Nui
{
    namespace Detail
    {
        struct WindowAccess;
    }

    enum class WebViewHint : std::int32_t
    {
        WEBVIEW_HINT_NONE,
        WEBVIEW_HINT_MIN,
        WEBVIEW_HINT_MAX,
        WEBVIEW_HINT_FIXED
    };

    // https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2?view=webview2-1.0.1370.28#corewebview2_host_resource_access_kind
    enum class HostResourceAccessKind
    {
        Deny,
        Allow,
        DenyCors
    };

    enum class NuiCoreWebView2WebResourceContext
    {
        All,
        Document,
        Stylesheet,
        Image,
        Media,
        Font,
        Script,
        XmlHttpRequest,
        Fetch,
        TextTrack,
        EventSource,
        WebSocket,
        Manifest,
        SignedExchange,
        Ping,
        CspViolationReport,
        Other,
    };

#ifdef NUI_BACKEND
    struct CustomSchemeRequest
    {
        std::string scheme{};
        std::variant<std::function<std::string const&()>, std::function<std::string()>> getContent;
        /// Pull-based streaming reader for the request body. Only populated when CustomScheme::streamingContent
        /// is true; empty (falsy std::function) otherwise. When populated, `getContent` is left empty and must
        /// not be invoked — read the body via this reader instead.
        ///
        /// Fills `buffer` with up to `bufferSize` bytes and returns the number of bytes written. A return value
        /// of 0 signals EOF. Partial reads (less than `bufferSize` without EOF) are permitted; call repeatedly
        /// until 0 is returned.
        ///
        /// LIFETIME — IMPORTANT: this reader (and `getContent`) is only guaranteed to be safe to invoke
        /// during the synchronous body of `CustomScheme::onRequest`. The backend retains the underlying
        /// request handle for the duration of these closures, but invoking them after onRequest has returned
        /// (e.g. from an asynchronous `bodyReader`) is undefined behavior. Read all body bytes you need
        /// before returning a CustomSchemeResponse.
        std::function<std::size_t(char* buffer, std::size_t bufferSize)> readContent;
        std::unordered_multimap<std::string, std::string> headers{};
        std::string uri{};
        std::string method{};

        std::optional<Url> parseUrl() const;

        /// WINDOWS ONLY
        NuiCoreWebView2WebResourceContext resourceContext = NuiCoreWebView2WebResourceContext::All;
    };

    /// Response returned from a CustomScheme::onRequest handler.
    ///
    /// The body may be supplied in one of three mutually exclusive forms. When more than one is populated,
    /// the backend uses the first one in this priority order:
    ///
    ///   1. `bodyFile`   — file path; streamed directly by the OS (best for files, no buffering).
    ///   2. `bodyReader` — pull-based callback; streamed chunk-by-chunk (best for generated / piped data).
    ///   3. `body`       — in-memory string (simplest, but the full payload must fit in memory).
    ///
    /// Pick exactly one. Setting multiple is not an error, but the lower-priority ones are silently ignored.
    struct CustomSchemeResponse
    {
        int statusCode;
        /// WINDOWS ONLY
        std::string reasonPhrase{};
        std::unordered_multimap<std::string, std::string> headers{};

        /// In-memory response body. Simple and zero-ceremony for small payloads, but the entire body must
        /// be buffered in memory before the response is emitted — on Windows it is additionally copied into
        /// a COM memory stream, so peak memory use is roughly 2× the body size. For payloads larger than a
        /// few MB prefer `bodyFile` or `bodyReader`. Ignored when `bodyFile` or `bodyReader` is set.
        std::string body{};

        /// Path to a file whose contents are streamed as the response body. Most efficient way to serve
        /// files — uses `SHCreateStreamOnFileEx` on Windows, `g_file_read` (GFileInputStream) on Linux, and
        /// a chunked `std::ifstream` dispatch on macOS. No intermediate copy of the file contents is made.
        /// If the `Content-Length` header is absent, the backend populates it from the file size.
        std::optional<std::filesystem::path> bodyFile{};

        /// Pull-based streaming reader for the response body. Use for non-file sources (generated output,
        /// sockets, decompressed streams, etc.). Fills `buffer` with up to `bufferSize` bytes. Returns the
        /// number of bytes written; a return value of 0 signals EOF. Partial reads (less than `bufferSize`
        /// without EOF) are permitted — the backend loops until 0 is returned. If the total size is known,
        /// set `Content-Length` in `headers`; otherwise the response will be delivered without it.
        ///
        /// THREADING: the backend may invoke this from any thread, but never concurrently with itself for
        /// the same response. Exceptions thrown from this callback are caught at the platform boundary and
        /// converted into a stream-read failure (the response is truncated). Avoid throwing if you can
        /// signal "no more bytes" by returning 0 instead.
        std::function<std::size_t(char* buffer, std::size_t bufferSize)> bodyReader{};
    };

    /**
     * @brief Thread that runs CustomScheme::onRequest.
     */
    enum class SchemeHandlerThread
    {
        /// The UI thread, one request at a time. What the native engines do, safe for state shared with RPC handlers.
        Ui,
        /// CEF ONLY: any engine worker thread, several requests at once. For handlers that are thread safe.
        Any,
    };

    struct CustomScheme
    {
        std::string scheme;
        /// You should probably allow some origin, or '*', or this will never do much.
        std::vector<std::string> allowedOrigins = {};

        std::function<CustomSchemeResponse(CustomSchemeRequest const&)> onRequest = {};

        /// WINDOWS ONLY
        NuiCoreWebView2WebResourceContext resourceContext = NuiCoreWebView2WebResourceContext::All;

        /// WINDOWS ONLY - Whether the sites with this scheme will be treated as a Secure Context like an HTTPS site.
        bool treatAsSecure = true;

        /// WINDOWS ONLY - URI contains an authority. like "scheme://AUTHORITY_HERE/path".
        bool hasAuthorityComponent = false;

        /// When true, the backend populates CustomSchemeRequest::readContent (pull-based streaming reader)
        /// instead of CustomSchemeRequest::getContent. Use for large request bodies that should not be buffered
        /// into memory in their entirety. Supported fully on Windows and Linux; on macOS the body is already
        /// fully buffered by the OS so this only offers API consistency, not memory savings.
        bool streamingContent = false;

        /// Thread that runs onRequest. The native engines always use the UI thread.
        SchemeHandlerThread handlerThread = SchemeHandlerThread::Ui;
    };

    struct WindowOptions
    {
        /// The title of the window.
        std::optional<std::string> title = std::nullopt;

        /// May open the dev tools?
        bool debug = false;

        /// Custom schemes to register. The handlers belong to this window, other windows can serve the same scheme
        /// names with their own handlers. On Windows all windows of a process share one WebView2 browser process, which
        /// needs the same registrations (scheme names, allowedOrigins, treatAsSecure, hasAuthorityComponent) in each of
        /// them, a window with different ones cannot be created.
        std::vector<CustomScheme> customSchemes = {};

        /// WINDOWS ONLY
        std::optional<std::string> browserArguments = std::nullopt;

        /// WINDOWS ONLY
        bool enableTrackingPrevention = true;

        /// WINDOWS ONLY
        std::optional<std::string> language = std::nullopt;

        /// WEBKIT ONLY (Linux & Mac)
        std::optional<std::string> folderMappingScheme = std::string{"assets"};

        // Called when a message from the view cannot be parsed or references an invalid function or has no id.
        std::function<void(std::string_view)> onRpcError = {};

        // Called when the frontend RpcClient uses "awaitRpcAvailable". This can be used to register all rpc functions
        // and then mark the initialized flag via the RpcHub. This way users can avoid setting init scripts for the view
        // and only execute scripts once, once the view is loaded.
        std::function<void()> onRpcAliveMessage = {};
    };
#else
    struct WindowOptions
    {};
#endif

    /**
     * @brief This class encapsulates the webview.
     */
    class Window
    {
      public:
        // Warning! app.example is intentional to avoid timeout issues.
        // https://github.com/MicrosoftEdge/WebView2Feedback/issues/2381
        constexpr static std::string_view windowsServeAuthority = "app.example";

        /**
         * @brief Construct a new Window object.
         */
        Window();

        /**
         * @brief Construct a new Window object.
         *
         * @param options Additional options.
         */
        explicit Window(WindowOptions const& options);

        /**
         * @brief Construct a new Window object.
         *
         * @param debug If true, the dev tools may be opened.
         * @param options Additional options.
         */
        [[deprecated]] explicit Window(bool debug);

        /**
         * @brief Construct a new Window object.
         *
         * @param title The title of the window.
         * @param debug If true, the dev tools may be opened.
         * @param options Additional options.
         */
        [[deprecated]] explicit Window(std::string const& title, bool debug = false);

        /**
         * @brief Construct a new Window object.
         *
         * @param title The title of the window.
         * @param debug If true, the dev tools may be opened.
         * @param options Additional options.
         */
        [[deprecated]] explicit Window(char const* title, bool debug = false);
        ~Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&);
        Window& operator=(Window&&);

        /**
         * @brief Set the Title of the window.
         *
         * @param title
         */
        void setTitle(std::string const& title);

        /**
         * @brief Sets the size of the window.
         *
         * @param width
         * @param height
         * @param hint
         */
        void setSize(std::int32_t width, std::int32_t height, WebViewHint hint = WebViewHint::WEBVIEW_HINT_NONE);

        /**
         * @brief Sets the position of the window
         *
         * @param x xCoordinate
         * @param y yCoordinate
         * @param (MacOS only) use setFrameOrigin instead of setFrameTopLeftPoint (see apple doc)
         */
        void setPosition(std::int32_t x, std::int32_t y, bool useFrameOrigin = true);

        /**
         * @brief Center the window on the primary display. Requires size to be set first.
         */
        void centerOnPrimaryDisplay();

        /**
         * @brief Navigate to url.
         *
         * @param url
         */
        void navigate(const std::string& url);

        /**
         * @brief Navigate to url.
         *
         * @param url
         */
        void navigate(char const* url);

#ifdef NUI_BACKEND
        /**
         * @brief Navigate to file.
         *
         * @param file path to an html file.
         */
        void navigate(const std::filesystem::path& file);
#endif

        /**
         * @brief Close the window and exit run.
         *
         * With the CEF engine run() returns once the last window has closed, other windows stay open.
         */
        void terminate();

#ifndef APPLE
        /**
         * @brief Open the dev tools.
         * @note This function is not available on MacOS.
         */
        void openDevTools();
#endif

#ifdef NUI_BACKEND
        /**
         * @brief Bind a function into the web context. These will be available under globalThis.nui_rpc.backend.NAME
         *
         * @param name The name of the function.
         * @param callback The function to bind.
         */
        void bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback);

        /**
         * @brief Unbind a function from the web context.
         *
         * @param name The name of the function.
         */
        void unbind(std::string const& name);

        boost::asio::any_io_executor getExecutor() const;

        /**
         * @brief Map a host name under the assets:// scheme to a folder (https:// on windows).
         *
         * This is emulated on linux and macos, via the assets scheme. Prefer to use custom schemes instead.
         * The scheme can be changed via folderMappingScheme in the WindowOptions.
         *
         * @param hostName The host name to map. like "assets://HOSTNAME/...".
         * @param folderPath The path to the directory to map into.
         * @param accessKind [WINDOWS ONLY] The access kind (depends on Cors).
         */
        void setVirtualHostNameToFolderMapping(
            std::string const& hostName,
            std::filesystem::path const& folderPath,
            HostResourceAccessKind accessKind = HostResourceAccessKind::Allow);

        /**
         * @brief Run the webview. This function blocks until the window is closed.
         */
        void run();

        /**
         * @brief Run a function on the main thread
         *
         * @param func
         */
        void dispatch(std::function<void()> func);

        /**
         * @brief Set page html from a string.
         *
         * @param html Page html.
         * @param windowsServeThroughAuthority [WINDOWS ONLY] If set, the page will be served through the given
         * authority via a custom webRequestHandler. This is useful for CORS on custom scheme handlers, which would get
         * rejected otherwise.
         */
        void setHtml(
            std::string_view html,
            std::optional<std::string> windowsServeThroughAuthority = std::string{windowsServeAuthority});

        /**
         * @brief Dump the page into a temporary file and then load it from there.
         *
         * @param html A string containing the html.
         */
        void setHtmlThroughFilesystem(std::string_view html);

        /**
         * @brief Run javascript in the window.
         *
         * @param js
         */
        void eval(std::string const& js);

        /**
         * @brief Run javascript in the window.
         *
         * @param js
         */
        void eval(char const* js);

        /**
         * @brief Run javascript in the window.
         * @param file path to a javascript file.
         */
        void eval(std::filesystem::path const& file);

        /**
         * @brief Calls globalThis.nui_rpc.frontend[name] with a JSON argument. Used by the RpcHub.
         *
         * A missing function is recorded in globalThis.nui_rpc.errors. Callable from any thread.
         *
         * @param name The name of the frontend function.
         * @param jsonArgument The argument as JSON text, or an empty string for no argument.
         */
        void callFrontend(std::string const& name, std::string const& jsonArgument);

        /**
         * @brief Place javascript in the window.
         *
         * @param js
         */
        void init(std::string const& js);

        /**
         * @brief Place javascript in the window.
         * @param file path to a javascript file.
         */
        void init(std::filesystem::path const& file);

#    ifdef NUI_ENGINE_CEF
        // The CEF engine has no GTK, WebKit or WKWebView objects. Use the typed accessors in <nui/cef/client_hooks.hpp>.
        void* getNativeWebView() = delete;
        void* getNativeWindow() = delete;
#    else
        /**
         * @brief Get a pointer to the underlying webview (ICoreWebView2* on windows, WEBKIT_WEB_VIEW on linux, id on
         * mac).
         *
         * @return void* Cast this pointer to the correct type depending on the OS.
         */
        void* getNativeWebView();

        /**
         * @brief Get a pointer to the underlying window (HWND on windows, GtkWidget* on linux, id on mac)
         *
         * @return void* Cast this pointer to the correct type depending on the OS.
         */
        void* getNativeWindow();
#    endif

        /**
         * @brief [LINUX ONLY] Enable/Disable console output from view in the console.
         */
        void setConsoleOutput(bool active);
#endif

        void runInJavascriptThread(std::function<void()>&& func);

      public:
        struct Implementation;
        struct WindowsImplementation;
        struct LinuxImplementation;
        struct MacOsImplementation;

      private:
        friend struct Detail::WindowAccess;
        std::shared_ptr<Implementation> impl_;
    };
}