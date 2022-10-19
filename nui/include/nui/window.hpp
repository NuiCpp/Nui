#pragma once

#include <nui/core.hpp>
#ifdef NUI_BACKEND
#    include <nlohmann/json.hpp>
#endif

#include <memory>
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

    class Window
    {
      public:
        Window();
        explicit Window(bool debug);
        explicit Window(std::string const& title, bool debug = false);
        explicit Window(char const* title, bool debug = false);
        ~Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&);
        Window& operator=(Window&&);

        void setTitle(std::string const& title);
        void setSize(int width, int height, WebViewHint hint);
        void navigate(const std::string& url);
        void terminate();
        void openDevTools();
#ifdef NUI_BACKEND
        void bind(std::string const& name, std::function<void(nlohmann::json const&)> const& callback);
        void asyncDispatch(std::function<void()> func);
        void setVirtualHostNameToFolderMapping(
            std::string const& hostName,
            std::filesystem::path const& folderPath,
            HostResourceAccessKind accessKind);
        void run();
        void setHtml(std::string_view html);
        void eval(std::string const& js);
#endif

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}