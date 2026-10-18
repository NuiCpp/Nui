#pragma once

#include <nui/cef/client_hooks.hpp>

#include <include/cef_client.h>

#include <memory>
#include <string_view>

namespace Nui::Detail::Cef
{
    /**
     * @brief What a window's client reports to the window. Called on the UI thread.
     */
    class BrowserEvents
    {
      public:
        virtual ~BrowserEvents() = default;
        virtual void onAfterCreated(CefRefPtr<CefBrowser> browser) = 0;
        virtual void onBeforeClose(CefRefPtr<CefBrowser> browser) = 0;
        virtual void onPost(std::string_view message) = 0;
        virtual bool isAppUrl(std::string const& url) const = 0;
        virtual bool consoleOutput() const = 0;
    };

    /**
     * @brief Browser process client of one window's browser.
     */
    class Client
        : public CefClient
        , public CefLifeSpanHandler
        , public CefDisplayHandler
        , public CefPermissionHandler
    {
      public:
        Client(std::shared_ptr<BrowserEvents> events, Nui::Cef::ClientHooks hooks);

        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
        CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
        CefRefPtr<CefPermissionHandler> GetPermissionHandler() override;
        CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;
        CefRefPtr<CefDragHandler> GetDragHandler() override;
        CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override;
        CefRefPtr<CefRequestHandler> GetRequestHandler() override;

        bool OnProcessMessageReceived(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefProcessId sourceProcess,
            CefRefPtr<CefProcessMessage> message) override;

        bool OnBeforePopup(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            int popupId,
            CefString const& targetUrl,
            CefString const& targetFrameName,
            WindowOpenDisposition targetDisposition,
            bool userGesture,
            CefPopupFeatures const& popupFeatures,
            CefWindowInfo& windowInfo,
            CefRefPtr<CefClient>& client,
            CefBrowserSettings& settings,
            CefRefPtr<CefDictionaryValue>& extraInfo,
            bool* noJavascriptAccess) override;
        void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
        bool DoClose(CefRefPtr<CefBrowser> browser) override;
        void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

        bool OnConsoleMessage(
            CefRefPtr<CefBrowser> browser,
            cef_log_severity_t level,
            CefString const& message,
            CefString const& source,
            int line) override;

        bool OnShowPermissionPrompt(
            CefRefPtr<CefBrowser> browser,
            uint64_t promptId,
            CefString const& requestingOrigin,
            uint32_t requestedPermissions,
            CefRefPtr<CefPermissionPromptCallback> callback) override;

      private:
        bool isOwnBrowser(CefRefPtr<CefBrowser> const& browser) const;

        std::shared_ptr<BrowserEvents> events_;
        Nui::Cef::ClientHooks const hooks_;
        /// Identifier of the window's own browser. DevTools and allowed popups share the client and are ignored.
        int browserId_{0};
        IMPLEMENT_REFCOUNTING(Client);
    };
}
