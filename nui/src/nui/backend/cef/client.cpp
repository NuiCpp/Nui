#include "client.hpp"
#include "engine_state.hpp"
#include "scheme_handler.hpp"
#include "common/messages.hpp"

#include <include/cef_parser.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>

namespace Nui::Detail::Cef
{
    Client::Client(std::shared_ptr<BrowserEvents> events, Nui::Cef::ClientHooks hooks)
        : events_{std::move(events)}
        , hooks_{std::move(hooks)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefLifeSpanHandler> Client::GetLifeSpanHandler()
    {
        return this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefDisplayHandler> Client::GetDisplayHandler()
    {
        return this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefPermissionHandler> Client::GetPermissionHandler()
    {
        if (hooks_.permissionHandler)
            return hooks_.permissionHandler;
        return this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefContextMenuHandler> Client::GetContextMenuHandler()
    {
        return hooks_.contextMenuHandler;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefDragHandler> Client::GetDragHandler()
    {
        return hooks_.dragHandler;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefKeyboardHandler> Client::GetKeyboardHandler()
    {
        return hooks_.keyboardHandler;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefRequestHandler> Client::GetRequestHandler()
    {
        // An application handler replaces Nui's, CEF's loader then applies Range requests on top of the scheme handler.
        if (hooks_.requestHandler)
            return hooks_.requestHandler;
        return this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefResourceRequestHandler> Client::GetResourceRequestHandler(
        CefRefPtr<CefBrowser>,
        CefRefPtr<CefFrame>,
        CefRefPtr<CefRequest> request,
        bool,
        bool,
        CefString const&,
        bool&)
    {
        if (request->GetHeaderByName("Range").empty())
            return nullptr;
        CefURLParts parts;
        if (!CefParseURL(request->GetURL(), parts))
            return nullptr;
        if (!EngineState::instance().isAppOrigin(CefString(&parts.scheme).ToString(), CefString(&parts.host).ToString()))
            return nullptr;
        return this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefResourceRequestHandler::ReturnValue Client::OnBeforeResourceLoad(
        CefRefPtr<CefBrowser>,
        CefRefPtr<CefFrame>,
        CefRefPtr<CefRequest> request,
        CefRefPtr<CefCallback>)
    {
        // Nui's scheme handler answers the range itself. CEF's loader would also apply it, rewrite Content-Length and
        // fail requests with ranges it cannot satisfy.
        CefRequest::HeaderMap headers;
        request->GetHeaderMap(headers);
        CefRequest::HeaderMap renamed;
        for (auto const& [name, value] : headers)
        {
            const auto text = name.ToString();
            const bool isRange = text.size() == 5u && (text[0] == 'R' || text[0] == 'r') &&
                std::equal(text.begin() + 1, text.end(), "ange", [](char lhs, char rhs) {
                    return std::tolower(static_cast<unsigned char>(lhs)) == rhs;
                });
            renamed.emplace(isRange ? CefString{rangeHeaderName} : name, value);
        }
        request->SetHeaderMap(renamed);
        return RV_CONTINUE;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::isOwnBrowser(CefRefPtr<CefBrowser> const& browser) const
    {
        return browser && browserId_ != 0 && browser->GetIdentifier() == browserId_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnProcessMessageReceived(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId,
        CefRefPtr<CefProcessMessage> message)
    {
        if (message->GetName() != postMessageName)
            return false;
        // Only the main frame of an app document may call into the backend.
        if (!events_ || !isOwnBrowser(browser) || !frame || !frame->IsMain() ||
            !events_->isAppUrl(frame->GetURL().ToString()))
            return true;
        visitPayload(message, [this](std::string_view payload) {
            try
            {
                events_->onPost(payload);
            }
            catch (std::exception const& exception)
            {
                std::cerr << "Nui: exception in message handler: " << exception.what() << std::endl;
            }
        });
        return true;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnBeforePopup(
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
        bool* noJavascriptAccess)
    {
        if (!hooks_.popupHandler)
            return true;
        return hooks_.popupHandler->OnBeforePopup(
            browser,
            frame,
            popupId,
            targetUrl,
            targetFrameName,
            targetDisposition,
            userGesture,
            popupFeatures,
            windowInfo,
            client,
            settings,
            extraInfo,
            noJavascriptAccess);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Client::OnAfterCreated(CefRefPtr<CefBrowser> browser)
    {
        if (browserId_ != 0 || !events_)
            return;
        browserId_ = browser->GetIdentifier();
        events_->onAfterCreated(browser);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::DoClose(CefRefPtr<CefBrowser>)
    {
        // Let the browser view close the window, OnBeforeClose follows.
        return false;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Client::OnBeforeClose(CefRefPtr<CefBrowser> browser)
    {
        if (!events_ || !isOwnBrowser(browser))
            return;
        const auto events = std::move(events_);
        events_.reset();
        events->onBeforeClose(browser);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnConsoleMessage(
        CefRefPtr<CefBrowser> browser,
        cef_log_severity_t,
        CefString const& message,
        CefString const& source,
        int line)
    {
        if (!events_ || !isOwnBrowser(browser) || !events_->consoleOutput())
            return true;
        std::cout << source.ToString() << ":" << line << ": " << message.ToString() << std::endl;
        return true;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnShowPermissionPrompt(
        CefRefPtr<CefBrowser>,
        uint64_t,
        CefString const& requestingOrigin,
        uint32_t requestedPermissions,
        CefRefPtr<CefPermissionPromptCallback> callback)
    {
        const bool onlyClipboard = (requestedPermissions & ~static_cast<uint32_t>(CEF_PERMISSION_TYPE_CLIPBOARD)) == 0u;
        const bool appOrigin = events_ && events_->isAppUrl(requestingOrigin.ToString());
        callback->Continue(onlyClipboard && appOrigin ? CEF_PERMISSION_RESULT_ACCEPT : CEF_PERMISSION_RESULT_DENY);
        return true;
    }
}
