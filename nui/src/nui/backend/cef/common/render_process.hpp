#pragma once

#include <include/cef_render_process_handler.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Nui::Detail::Cef
{
    /**
     * @brief Renderer side of the Nui windows: __webview__.post, init scripts and the ordered browser messages.
     */
    class RenderProcess : public CefRenderProcessHandler
    {
      public:
        /**
         * @param processAppHosts setHtml authorities the browser process knew when it started this process.
         */
        explicit RenderProcess(std::vector<std::string> processAppHosts);

        void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extraInfo) override;
        void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;
        void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
            override;
        void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
            override;
        bool OnProcessMessageReceived(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefProcessId sourceProcess,
            CefRefPtr<CefProcessMessage> message) override;

      private:
        struct PendingOperation
        {
            std::string name;
            std::string payload;
        };

        struct BrowserState
        {
            int references{0};
            bool contextReady{false};
            std::vector<std::string> initScripts{};
            std::vector<std::string> appSchemes{};
            std::vector<std::string> appHosts{};
            std::vector<PendingOperation> pending{};
        };

        bool isAppDocument(BrowserState const& state, CefRefPtr<CefFrame> const& frame) const;
        void runOperation(CefRefPtr<CefFrame> const& frame, std::string const& name, std::string_view payload);

        std::vector<std::string> processAppHosts_;
        std::unordered_map<int, BrowserState> browsers_{};
        IMPLEMENT_REFCOUNTING(RenderProcess);
    };
}
