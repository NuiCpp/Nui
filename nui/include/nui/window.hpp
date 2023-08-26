#pragma once

#include <nui/core.hpp>
#ifdef NUI_BACKEND
#    include <nlohmann/json.hpp>
#    include <boost/asio/any_io_executor.hpp>
#endif

#include <memory>
#include <optional>
#include <string>
#include <functional>
#include <filesystem>

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

    struct WindowOptions
    {
        /// The title of the window.
        std::optional<std::string> title = std::nullopt;

        /// May open the dev tools?
        bool debug = false;

        /// WINDOWS ONLY
        std::optional<std::string> browserArguments = std::nullopt;
    };

    /**
     * @brief This class encapsulates the webview.
     */
    class Window
    {
      public:
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
         */
        void setPosition(int x, int y);

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

        /**
         * @brief Open the dev tools.
         */
        void openDevTools();
#ifdef NUI_BACKEND
        /**
         * @brief Bind a function into the web context. These will be available under globalThis.nui_rpc.backend.NAME
         *
         * @param name The name of the function.
         * @param callback The function to bind.
         */
        void bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback);

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
         */
        void setHtml(std::string_view html);

        /**
         * @brief Run javascript in the window.
         *
         * @param js
         */
        void eval(std::string const& js);

        /**
         * @brief Get a pointer to the underlying webview (ICoreWebView2* on windows and WEBKIT_WEB_VIEW on linux.
         *
         * @return void* Cast this pointer to the correct type depending on the OS.
         */
        void* getNativeWebView();

        /**
         * @brief [LINUX ONLY] Enable/Disable console output from view in the console.
         */
        void setConsoleOutput(bool active);
#endif

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}