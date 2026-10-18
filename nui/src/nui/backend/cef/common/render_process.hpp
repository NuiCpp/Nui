#pragma once

#include <include/cef_render_process_handler.h>

#include <cstdint>
#include <deque>
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
            /// Scripts the browser sent before this operation, they run first.
            std::uint32_t scriptsBefore;
        };

        struct BrowserState
        {
            int references{0};
            bool contextReady{false};
            bool stateRequested{false};
            bool stateRequestPending{false};
            /// By their index in the window's list, which keeps the order and drops duplicates.
            std::map<std::uint32_t, std::string> initScripts{};
            /// Scripts below this index have all arrived. Later ones, and operations sent after them, wait for the gap
            /// to fill, in a new process the state reply fills it.
            std::uint32_t availableScripts{0u};
            /// Scripts that ran in the current document.
            std::uint32_t evaluatedScripts{0u};
            std::vector<std::string> appSchemes{};
            std::vector<std::string> appHosts{};
            /// Calls and evals in arrival order, waiting for the document or for earlier scripts.
            std::deque<PendingOperation> pending{};
        };

        bool isAppDocument(BrowserState const& state, CefRefPtr<CefFrame> const& frame) const;
        static void advanceAvailableScripts(BrowserState& state);
        /// Asks the browser for the window's state unless an answer is pending.
        static void requestState(BrowserState& state, CefRefPtr<CefFrame> const& frame);
        /// True when a script below the highest known index is missing.
        static bool missingScripts(BrowserState const& state);
        /// Runs available scripts and pending operations of the current document in the order the browser sent them.
        void drain(BrowserState& state, CefRefPtr<CefFrame> const& frame, CefRefPtr<CefV8Context> const& context);
        void runOperation(CefRefPtr<CefFrame> const& frame, std::string const& name, std::string_view payload);

        std::unordered_map<int, BrowserState> browsers_{};
        IMPLEMENT_REFCOUNTING(RenderProcess);
    };
}
