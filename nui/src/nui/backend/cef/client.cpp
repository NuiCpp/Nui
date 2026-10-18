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
    namespace
    {
        bool isRangeHeader(std::string const& name)
        {
            return name.size() == 5u && std::equal(name.begin(), name.end(), "range", [](char lhs, char rhs) {
                       return std::tolower(static_cast<unsigned char>(lhs)) == rhs;
                   });
        }

        void renameRangeHeader(CefRefPtr<CefRequest> const& request)
        {
            CefRequest::HeaderMap headers;
            request->GetHeaderMap(headers);
            CefRequest::HeaderMap renamed;
            for (auto const& [name, value] : headers)
                renamed.emplace(isRangeHeader(name.ToString()) ? CefString{rangeHeaderName} : name, value);
            request->SetHeaderMap(renamed);
        }

        /**
         * @brief Continues an application's asynchronous OnBeforeResourceLoad, renaming the Range header first.
         */
        class RenamingCallback : public CefCallback
        {
          public:
            RenamingCallback(CefRefPtr<CefCallback> callback, CefRefPtr<CefRequest> request)
                : callback_{std::move(callback)}
                , request_{std::move(request)}
            {}

            void Continue() override
            {
                renameRangeHeader(request_);
                callback_->Continue();
            }

            void Cancel() override
            {
                callback_->Cancel();
            }

          private:
            CefRefPtr<CefCallback> callback_;
            CefRefPtr<CefRequest> request_;
            IMPLEMENT_REFCOUNTING(RenamingCallback);
        };

        /**
         * @brief Handles a Range request to an app origin. Nui's scheme handlers answer the range themselves, CEF's
         * loader would apply it again, rewrite Content-Length and fail ranges it cannot satisfy. Everything else goes
         * to the application's handler, whose OnBeforeResourceLoad still sees the Range header.
         */
        class AppRangeHandler : public CefResourceRequestHandler
        {
          public:
            explicit AppRangeHandler(CefRefPtr<CefResourceRequestHandler> application)
                : application_{std::move(application)}
            {}

            CefRefPtr<CefCookieAccessFilter>
            GetCookieAccessFilter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request)
                override
            {
                return application_ ? application_->GetCookieAccessFilter(browser, frame, request) : nullptr;
            }

            ReturnValue OnBeforeResourceLoad(
                CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefRequest> request,
                CefRefPtr<CefCallback> callback) override
            {
                if (!application_)
                {
                    renameRangeHeader(request);
                    return RV_CONTINUE;
                }
                const auto result =
                    application_->OnBeforeResourceLoad(browser, frame, request, new RenamingCallback(callback, request));
                if (result == RV_CONTINUE)
                    renameRangeHeader(request);
                return result;
            }

            CefRefPtr<CefResourceHandler>
            GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request)
                override
            {
                return application_ ? application_->GetResourceHandler(browser, frame, request) : nullptr;
            }

            void OnResourceRedirect(
                CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefRequest> request,
                CefRefPtr<CefResponse> response,
                CefString& newUrl) override
            {
                if (application_)
                    application_->OnResourceRedirect(browser, frame, request, response, newUrl);
            }

            bool OnResourceResponse(
                CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefRequest> request,
                CefRefPtr<CefResponse> response) override
            {
                return application_ && application_->OnResourceResponse(browser, frame, request, response);
            }

            CefRefPtr<CefResponseFilter> GetResourceResponseFilter(
                CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefRequest> request,
                CefRefPtr<CefResponse> response) override
            {
                return application_ ? application_->GetResourceResponseFilter(browser, frame, request, response)
                                    : nullptr;
            }

            void OnResourceLoadComplete(
                CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefRequest> request,
                CefRefPtr<CefResponse> response,
                URLRequestStatus status,
                int64_t receivedContentLength) override
            {
                if (application_)
                    application_->OnResourceLoadComplete(browser, frame, request, response, status, receivedContentLength);
            }

            void OnProtocolExecution(
                CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefRequest> request,
                bool& allowOsExecution) override
            {
                if (application_)
                    application_->OnProtocolExecution(browser, frame, request, allowOsExecution);
            }

          private:
            CefRefPtr<CefResourceRequestHandler> application_;
            IMPLEMENT_REFCOUNTING(AppRangeHandler);
        };
    }
    //---------------------------------------------------------------------------------------------------------------------
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
        return this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnBeforeBrowse(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool userGesture,
        bool isRedirect)
    {
        return hooks_.requestHandler &&
            hooks_.requestHandler->OnBeforeBrowse(browser, frame, request, userGesture, isRedirect);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnOpenURLFromTab(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefString const& targetUrl,
        WindowOpenDisposition targetDisposition,
        bool userGesture)
    {
        return hooks_.requestHandler &&
            hooks_.requestHandler->OnOpenURLFromTab(browser, frame, targetUrl, targetDisposition, userGesture);
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefResourceRequestHandler> Client::GetResourceRequestHandler(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefRequest> request,
        bool isNavigation,
        bool isDownload,
        CefString const& requestInitiator,
        bool& disableDefaultHandling)
    {
        CefRefPtr<CefResourceRequestHandler> application;
        if (hooks_.requestHandler)
        {
            application = hooks_.requestHandler->GetResourceRequestHandler(
                browser, frame, request, isNavigation, isDownload, requestInitiator, disableDefaultHandling);
        }
        // The app's OnBeforeResourceLoad may add a Range header, so with a handler every app request is wrapped.
        if (!hooks_.requestHandler && request->GetHeaderByName("Range").empty())
            return application;
        CefURLParts parts;
        if (!CefParseURL(request->GetURL(), parts) ||
            !EngineState::instance().isAppOrigin(CefString(&parts.scheme).ToString(), CefString(&parts.host).ToString()))
            return application;
        return new AppRangeHandler(application);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::GetAuthCredentials(
        CefRefPtr<CefBrowser> browser,
        CefString const& originUrl,
        bool isProxy,
        CefString const& host,
        int port,
        CefString const& realm,
        CefString const& scheme,
        CefRefPtr<CefAuthCallback> callback)
    {
        return hooks_.requestHandler &&
            hooks_.requestHandler->GetAuthCredentials(browser, originUrl, isProxy, host, port, realm, scheme, callback);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnCertificateError(
        CefRefPtr<CefBrowser> browser,
        cef_errorcode_t certificateError,
        CefString const& requestUrl,
        CefRefPtr<CefSSLInfo> sslInfo,
        CefRefPtr<CefCallback> callback)
    {
        return hooks_.requestHandler &&
            hooks_.requestHandler->OnCertificateError(browser, certificateError, requestUrl, sslInfo, callback);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnSelectClientCertificate(
        CefRefPtr<CefBrowser> browser,
        bool isProxy,
        CefString const& host,
        int port,
        X509CertificateList const& certificates,
        CefRefPtr<CefSelectClientCertificateCallback> callback)
    {
        return hooks_.requestHandler &&
            hooks_.requestHandler->OnSelectClientCertificate(browser, isProxy, host, port, certificates, callback);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Client::OnRenderViewReady(CefRefPtr<CefBrowser> browser)
    {
        if (hooks_.requestHandler)
            hooks_.requestHandler->OnRenderViewReady(browser);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Client::OnRenderProcessUnresponsive(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefUnresponsiveProcessCallback> callback)
    {
        return hooks_.requestHandler && hooks_.requestHandler->OnRenderProcessUnresponsive(browser, callback);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Client::OnRenderProcessResponsive(CefRefPtr<CefBrowser> browser)
    {
        if (hooks_.requestHandler)
            hooks_.requestHandler->OnRenderProcessResponsive(browser);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Client::OnRenderProcessTerminated(
        CefRefPtr<CefBrowser> browser,
        TerminationStatus status,
        int errorCode,
        CefString const& errorString)
    {
        if (hooks_.requestHandler)
            hooks_.requestHandler->OnRenderProcessTerminated(browser, status, errorCode, errorString);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Client::OnDocumentAvailableInMainFrame(CefRefPtr<CefBrowser> browser)
    {
        if (hooks_.requestHandler)
            hooks_.requestHandler->OnDocumentAvailableInMainFrame(browser);
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
        const auto name = message->GetName().ToString();
        if (name == stateRequestMessageName)
        {
            if (events_ && isOwnBrowser(browser) && frame && frame->IsMain())
                events_->onStateRequest();
            return true;
        }
        if (name != postMessageName)
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
