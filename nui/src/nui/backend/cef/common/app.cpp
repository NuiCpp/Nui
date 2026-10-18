#include "app.hpp"
#include "messages.hpp"

#include <include/cef_scheme.h>

#include <algorithm>
#include <string_view>

namespace Nui::Detail::Cef
{
    int customSchemeOptions()
    {
        return CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_SECURE | CEF_SCHEME_OPTION_CORS_ENABLED |
            CEF_SCHEME_OPTION_FETCH_ENABLED;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::vector<std::string> customSchemesFromCommandLine(CefRefPtr<CefCommandLine> const& commandLine)
    {
        std::vector<std::string> schemes;
        if (!commandLine || !commandLine->HasSwitch(customSchemesSwitch))
            return schemes;
        const auto value = commandLine->GetSwitchValue(customSchemesSwitch).ToString();
        std::string_view rest{value};
        while (!rest.empty())
        {
            const auto comma = rest.find(',');
            const auto scheme = rest.substr(0, comma);
            if (!scheme.empty() && std::ranges::find(schemes, scheme) == schemes.end())
                schemes.emplace_back(scheme);
            if (comma == std::string_view::npos)
                break;
            rest.remove_prefix(comma + 1);
        }
        return schemes;
    }
    //---------------------------------------------------------------------------------------------------------------------
    App::App(
        std::vector<std::string> customSchemes,
        CefRefPtr<CefBrowserProcessHandler> browserProcessHandler,
        CefRefPtr<CefRenderProcessHandler> renderProcessHandler,
        CommandLineHook commandLineHook)
        : customSchemes_{std::move(customSchemes)}
        , browserProcessHandler_{std::move(browserProcessHandler)}
        , renderProcessHandler_{std::move(renderProcessHandler)}
        , commandLineHook_{std::move(commandLineHook)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void App::OnBeforeCommandLineProcessing(CefString const& processType, CefRefPtr<CefCommandLine> commandLine)
    {
        if (commandLineHook_)
            commandLineHook_(processType, commandLine);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void App::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
    {
        for (auto const& scheme : customSchemes_)
            registrar->AddCustomScheme(scheme, customSchemeOptions());
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefBrowserProcessHandler> App::GetBrowserProcessHandler()
    {
        return browserProcessHandler_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefRenderProcessHandler> App::GetRenderProcessHandler()
    {
        return renderProcessHandler_;
    }
}
