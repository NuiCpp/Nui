#pragma once

#include <include/cef_render_process_handler.h>

#include <cstdint>
#include <map>
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
            bool stateRequested{false};
            bool stateRequestPending{false};
            /// By their index in the window's list, which keeps the order and drops duplicates.
            std::map<std::uint32_t, std::string> initScripts{};
            /// Scripts below this index have all arrived and run in documents. Later ones wait for the gap to fill,
            /// in a new process the state reply fills it.
            std::uint32_t contiguousScripts{0u};
            std::vector<std::string> appSchemes{};
            std::vector<std::string> appHosts{};
            std::vector<PendingOperation> pending{};
        };

        bool isAppDocument(BrowserState const& state, CefRefPtr<CefFrame> const& frame) const;
        /// Advances contiguousScripts, returns the scripts that became runnable in index order.
        static std::vector<std::string const*> advanceScripts(BrowserState& state);
        /// Asks the browser for the window's state unless an answer is pending.
        static void requestState(BrowserState& state, CefRefPtr<CefFrame> const& frame);
        /// True when a script below the highest known index is missing.
        static bool missingScripts(BrowserState const& state);
        void runOperation(CefRefPtr<CefFrame> const& frame, std::string const& name, std::string_view payload);

        std::unordered_map<int, BrowserState> browsers_{};
        IMPLEMENT_REFCOUNTING(RenderProcess);
    };
}
