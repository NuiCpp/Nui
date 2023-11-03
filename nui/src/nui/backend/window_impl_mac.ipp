struct Window::MacOsImplementation : public Window::Implementation
{
    void registerSchemeHandlers(WindowOptions const& options) override
    {
        // TODO:
    }

    MacOsImplementation(WindowOptions const& options)
        : Implementation{options}
    {}
};