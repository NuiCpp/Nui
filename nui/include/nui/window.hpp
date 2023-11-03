#pragma once

#include <nui/core.hpp>
#ifdef NUI_BACKEND
#    include <nlohmann/json.hpp>
#    include <boost/asio/any_io_executor.hpp>
#    include <nui/backend/url.hpp>
#    include <filesystem>
#endif

#include <memory>
#include <optional>
#include <string>
#include <functional>
#include <unordered_map>

namespace Nui
{
    enum class WebViewHint : int
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
        std::string scheme;
        std::function<std::string const&()> getContent;
        std::unordered_multimap<std::string, std::string> headers;
        std::string uri;
        std::string method;

        std::optional<Url> parseUrl() const;

        // WINDOWS ONLY
        NuiCoreWebView2WebResourceContext resourceContext;
    };

    struct CustomSchemeResponse
    {
        int statusCode;
        std::string reasonPhrase;
        std::unordered_multimap<std::string, std::string> headers;
        std::string body;
    };

    struct CustomScheme
    {
        std::string scheme;
        std::vector<std::string> allowedOrigins = {};
        std::function<CustomSchemeResponse(CustomSchemeRequest const&)> onRequest = {};

        /// WINDOWS ONLY
        NuiCoreWebView2WebResourceContext resourceContext = NuiCoreWebView2WebResourceContext::All;

        /// WINDOWS ONLY - Whether the sites with this scheme will be treated as a Secure Context like an HTTPS site.
        bool treatAsSecure = true;

        /// WINDOWS ONLY - URI contains an authority. like "scheme://AUTHORITY_HERE/path".
        bool hasAuthorityComponent = false;
    };

    struct WindowOptions
    {
        /// The title of the window.
        std::optional<std::string> title = std::nullopt;

        /// May open the dev tools?
        bool debug = false;

        /// Custom schemes to register.
        std::vector<CustomScheme> customSchemes = {};

        /// WINDOWS ONLY
        std::optional<std::string> browserArguments = std::nullopt;

        /// WINDOWS ONLY
        bool enableTrackingPrevention = true;

        /// WINDOWS ONLY
        std::optional<std::string> language = std::nullopt;

        /// WEBKIT ONLY
        std::string localScheme = "assets";
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
        constexpr static std::string_view windowsServeAuthority = "nuilocal";

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
        void setSize(int width, int height, WebViewHint hint = WebViewHint::WEBVIEW_HINT_NONE);

        /**
         * @brief Sets the position of the window
         *
         * @param x xCoordinate
         * @param y yCoordinate
         * @param (MacOS only) use setFrameOrigin instead of setFrameTopLeftPoint (see apple doc)
         */
        void setPosition(int x, int y, bool useFrameOrigin = true);

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
         * @brief Map a host name under the assets:// scheme to a folder.
         *
         * @param hostName The host name to map. like "assets://HOSTNAME/...".
         * @param folderPath The path to the directory to map into.
         * @param accessKind [WINDOWS ONLY] The access kind (depends on Cors).
         */
        void setVirtualHostNameToFolderMapping(
            std::string const& hostName,
            std::filesystem::path const& folderPath,
            HostResourceAccessKind accessKind);

        /**
         * @brief Run the webview. This function blocks until the window is closed.
         */
        void run();

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
         * @brief Deprecated. Use setHtml or setHtmlThroughFilesystem instead.
         */
        [[deprecated]] void setHtml(
            std::string_view html,
            bool fromFilesystem = false,
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

        /**
         * @brief [LINUX ONLY] Enable/Disable console output from view in the console.
         */
        void setConsoleOutput(bool active);
#endif

      private:
        void runInJavascriptThread(std::function<void()>&& func);

      public:
        struct Implementation;
        struct WindowsImplementation;
        struct LinuxImplementation;
        struct MacOsImplementation;

      private:
        std::shared_ptr<Implementation> impl_;
    };
}