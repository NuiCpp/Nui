#pragma once

#include <include/cef_scheme.h>

#include <string>

namespace Nui::Detail::Cef
{
    /// Name Nui's request handler gives the Range header of app requests, so CEF's loader does not apply the range on
    /// top of Nui's own handling (see AppRangeHandler in client.cpp).
    inline constexpr char const* rangeHeaderName = "X-Nui-Range";

    /**
     * @brief Serves a Nui custom scheme. One factory per scheme name, requests go to the window of their browser.
     */
    class SchemeHandlerFactory : public CefSchemeHandlerFactory
    {
      public:
        explicit SchemeHandlerFactory(std::string scheme);

        CefRefPtr<CefResourceHandler> Create(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefString const& schemeName,
            CefRefPtr<CefRequest> request) override;

      private:
        std::string scheme_;
        IMPLEMENT_REFCOUNTING(SchemeHandlerFactory);
    };

    /**
     * @brief Serves the page given to Window::setHtml at https://<authority>/index.html and answers every other request
     * for the authority with 404, so none of them reaches the network.
     */
    class HtmlHandlerFactory : public CefSchemeHandlerFactory
    {
      public:
        CefRefPtr<CefResourceHandler> Create(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefString const& schemeName,
            CefRefPtr<CefRequest> request) override;

      private:
        IMPLEMENT_REFCOUNTING(HtmlHandlerFactory);
    };
}
