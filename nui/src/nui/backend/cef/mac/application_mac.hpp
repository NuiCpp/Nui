#pragma once

#include <include/internal/cef_types_mac.h>

#include <cstdint>
#include <functional>

namespace Nui::Detail::Cef
{
    /**
     * @brief Loads the CEF framework of the app bundle, makes NSApp a CefAppProtocol application and installs the menu.
     * Throws when the executable is not inside an app bundle with the framework.
     */
    void prepareMacApplication();

    /**
     * @brief NSWindow* of a CEF window handle (an NSView*).
     */
    std::uintptr_t windowOfView(cef_window_handle_t handle);

    /**
     * @brief Runs the function inside an autorelease pool.
     */
    void withAutoreleasePool(std::function<void()> const& function);
}
