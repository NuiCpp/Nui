#pragma once

#include <string>

namespace Nui
{
    /**
     * @brief What configureWebKitGtkRendering did with the WebKitGTK DMA-BUF renderer.
     */
    enum class DmabufRendererDecision
    {
        /** @brief Not a WebKitGTK platform, nothing was changed. */
        NotApplicable,
        /** @brief A rendering variable was already set, the environment was left as it is. */
        KeptEnvironment,
        /** @brief The system looks safe, the renderer stays enabled. */
        Enabled,
        /** @brief WEBKIT_DISABLE_DMABUF_RENDERER=1 was set. */
        Disabled,
    };

    /**
     * @brief Result of configureWebKitGtkRendering.
     */
    struct WebKitGtkRenderingConfiguration
    {
        /** @brief The decision taken. */
        DmabufRendererDecision decision;
        /** @brief Human readable decision and its reason, meant for logging. */
        std::string reason;
    };

    /**
     * @brief Disables the WebKitGTK DMA-BUF renderer unless the system is known to handle it.
     *
     * The renderer shows blank or broken windows on several setups, NVIDIA most prominently. It stays enabled only
     * if the session is X11 or Wayland, software rendering is not forced, no NVIDIA display device is present, every
     * DRM card is driven by amdgpu, i915 or xe and one of their render nodes is accessible. Anything that cannot be
     * determined disables it, a slow window is preferred over one that does not render.
     *
     * If WEBKIT_DISABLE_DMABUF_RENDERER, WEBKIT_DMABUF_RENDERER_FORCE_SHM, WEBKIT_DISABLE_COMPOSITING_MODE or
     * __NV_DISABLE_EXPLICIT_SYNC is already set, nothing is changed. WEBKIT_DISABLE_DMABUF_RENDERER=0 therefore forces
     * the renderer on.
     *
     * Call it at the top of main, before GTK or WebKit are initialized and before other threads exist, it uses setenv.
     * Does nothing on Windows and macOS.
     *
     * @return The decision and its reason.
     */
    WebKitGtkRenderingConfiguration configureWebKitGtkRendering();
}
