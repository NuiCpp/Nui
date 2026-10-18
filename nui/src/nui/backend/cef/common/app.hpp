#pragma once

#include <include/cef_app.h>

#include <functional>
#include <string>
#include <vector>

namespace Nui::Detail::Cef
{
    /**
     * @brief Registration flags of every Nui custom scheme: standard (hierarchical URLs), secure, CORS and fetch.
     */
    int customSchemeOptions();

    /**
     * @brief Reads the scheme names the browser process passed with --nui-custom-schemes.
     */
    std::vector<std::string> customSchemesFromCommandLine(CefRefPtr<CefCommandLine> const& commandLine);

    /**
     * @brief The CefApp of the browser process and of the helper processes.
     */
    class App : public CefApp
    {
      public:
        using CommandLineHook = std::function<void(CefString const& processType, CefRefPtr<CefCommandLine>)>;

        /**
         * @param customSchemes Custom scheme names registered in this process.
         * @param browserProcessHandler Handler of the browser process, null in helpers.
         * @param renderProcessHandler Handler of renderer processes, null in the browser process.
         * @param commandLineHook Adjusts the command line before Chromium processes it.
         */
        App(std::vector<std::string> customSchemes,
            CefRefPtr<CefBrowserProcessHandler> browserProcessHandler,
            CefRefPtr<CefRenderProcessHandler> renderProcessHandler,
            CommandLineHook commandLineHook);

        void
        OnBeforeCommandLineProcessing(CefString const& processType, CefRefPtr<CefCommandLine> commandLine) override;
        void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
        CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;

      private:
        std::vector<std::string> customSchemes_;
        CefRefPtr<CefBrowserProcessHandler> browserProcessHandler_;
        CefRefPtr<CefRenderProcessHandler> renderProcessHandler_;
        CommandLineHook commandLineHook_;
        IMPLEMENT_REFCOUNTING(App);
    };
}
