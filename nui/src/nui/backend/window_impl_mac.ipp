namespace Nui
{
    // #####################################################################################################################
    struct Window::MacOsImplementation : public Window::Implementation
    {
        HostNameMappingInfo hostNameMappingInfo;

        void registerSchemeHandlers(WindowOptions const& options) override;

        MacOsImplementation()
            : Implementation{}
        {}
    };
    //---------------------------------------------------------------------------------------------------------------------
    void Window::MacOsImplementation::registerSchemeHandlers(WindowOptions const& /*options*/)
    {
        // Done in intialization of the webview
    }
    // #####################################################################################################################
}