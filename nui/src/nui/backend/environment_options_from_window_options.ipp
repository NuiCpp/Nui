// #####################################################################################################################
namespace Nui
{
    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions>
    webView2EnvironmentOptionsFromOptions(WindowOptions const& options)
    {
        auto environmentOptions = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();

        if (options.browserArguments)
        {
            const auto wideArgs = widenString(*options.browserArguments);
            environmentOptions->put_AdditionalBrowserArguments(wideArgs.c_str());
        }

        if (options.language)
        {
            const auto wideLanguage = widenString(*options.language);
            environmentOptions->put_Language(wideLanguage.c_str());
        }

        Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options4;
        if (environmentOptions.As(&options4) == S_OK)
        {
            std::vector<Microsoft::WRL::ComPtr<CoreWebView2CustomSchemeRegistration>> customSchemeRegistrations;
            std::vector<std::vector<std::wstring>> allowedOrigins;
            std::vector<std::vector<std::wstring::value_type const*>> allowedOriginsRaw;
            std::vector<std::wstring> wideSchemes;

            allowedOrigins.reserve(options.customSchemes.size());
            allowedOriginsRaw.reserve(options.customSchemes.size());
            wideSchemes.reserve(options.customSchemes.size());

            for (const auto& customScheme : options.customSchemes)
            {
                wideSchemes.push_back(widenString(customScheme.scheme));
                customSchemeRegistrations.push_back(
                    Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(wideSchemes.back().c_str()));
                auto& customSchemeRegistration = customSchemeRegistrations.back();

                allowedOrigins.push_back({});
                allowedOrigins.back().reserve(customScheme.allowedOrigins.size());
                for (const auto& allowedOrigin : customScheme.allowedOrigins)
                    allowedOrigins.back().push_back(widenString(allowedOrigin));

                allowedOriginsRaw.push_back({});
                allowedOriginsRaw.back().reserve(allowedOrigins.back().size());
                for (const auto& allowedOrigin : allowedOrigins.back())
                    allowedOriginsRaw.back().push_back(allowedOrigin.c_str());

                customSchemeRegistration->SetAllowedOrigins(
                    static_cast<UINT>(allowedOriginsRaw.back().size()), allowedOriginsRaw.back().data());
                customSchemeRegistration->put_TreatAsSecure(customScheme.treatAsSecure);
                customSchemeRegistration->put_HasAuthorityComponent(customScheme.hasAuthorityComponent);
            }
            std::vector<ICoreWebView2CustomSchemeRegistration*> customSchemeRegistrationsRaw;
            customSchemeRegistrationsRaw.reserve(customSchemeRegistrations.size());
            for (const auto& customSchemeRegistration : customSchemeRegistrations)
                customSchemeRegistrationsRaw.push_back(customSchemeRegistration.Get());

            const auto result = options4->SetCustomSchemeRegistrations(
                static_cast<UINT>(customSchemeRegistrationsRaw.size()), customSchemeRegistrationsRaw.data());
            if (FAILED(result))
                throw std::runtime_error("Could not set custom scheme registrations.");
        }

        Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions5> options5;
        if (environmentOptions.As(&options5) == S_OK)
        {
            options5->put_EnableTrackingPrevention(options.enableTrackingPrevention);
        }

        return environmentOptions;
    }
}
// #####################################################################################################################