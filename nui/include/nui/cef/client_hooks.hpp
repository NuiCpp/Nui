#pragma once

// CEF specific access to a Nui::Window. Link the Nui::cef target to use this header.

#include <nui/window.hpp>

#include <include/cef_browser.h>
#include <include/cef_context_menu_handler.h>
#include <include/cef_drag_handler.h>
#include <include/cef_keyboard_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_permission_handler.h>
#include <include/cef_request_handler.h>

#include <cstdint>
#include <string>

namespace Nui::Cef
{
    /**
     * @brief Optional CEF handlers of a window. Nui's own client forwards to them.
     */
    struct ClientHooks
    {
        CefRefPtr<CefContextMenuHandler> contextMenuHandler{};
        CefRefPtr<CefDragHandler> dragHandler{};
        CefRefPtr<CefKeyboardHandler> keyboardHandler{};
        /// Replaces Nui's policy, which grants clipboard access to the app's own pages and denies everything else.
        CefRefPtr<CefPermissionHandler> permissionHandler{};
        CefRefPtr<CefRequestHandler> requestHandler{};
        /// Only OnBeforePopup is forwarded. Without it popups (target=_blank, window.open) are blocked.
        CefRefPtr<CefLifeSpanHandler> popupHandler{};
    };

    /**
     * @brief Sets the hooks of a window. Takes effect for the window's browser, so call it before Window::run, or,
     * while the engine runs, in the same callback that constructed the window.
     */
    void setClientHooks(Window& window, ClientHooks hooks);

    /**
     * @brief The browser of the window, null before it exists and after it closed. Call on the UI thread.
     */
    CefRefPtr<CefBrowser> browser(Window& window);

    struct NativeWindowHandle
    {
        enum class Kind
        {
            None,
            /// An X11 window id.
            X11,
            /// An opaque CEF handle, Wayland offers no global window id.
            Wayland,
            /// An NSWindow*.
            MacWindow,
        };
        Kind kind = Kind::None;
        std::uintptr_t value = 0;
    };

    /**
     * @brief Native handle of the window's top level window. Empty before the browser exists. Call on the UI thread.
     */
    NativeWindowHandle nativeWindowHandle(Window& window);

    /**
     * @brief Parent window identifier for xdg-desktop-portal calls, "x11:<hex id>", or empty when there is none, which is
     * the case on Wayland.
     */
    std::string portalParentWindow(Window& window);
}
