#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Nui
{
    /**
     * @brief Sandbox policy of the CEF engine on Linux. Ignored by the native engines.
     */
    enum class SandboxPolicy
    {
        /// Use Chromium's sandbox when the system supports it, otherwise run without it and log a warning.
        Auto,
        /// Fail to start the engine when no sandbox is available.
        Required,
        /// Never sandbox the child processes.
        Disabled,
    };

    /**
     * @brief Process wide browser engine settings. Only the CEF engine uses them.
     */
    struct EngineOptions
    {
        /// Names of custom schemes of windows created after the engine started. Schemes of windows constructed before
        /// the engine starts are registered automatically.
        std::vector<std::string> customSchemes = {};

        /// Extra Chromium switches like "--enable-features=Name". Feature lists are merged with Nui's defaults.
        std::vector<std::string> browserArguments = {};

        /// Port of the Chrome DevTools Protocol endpoint. 0 picks a free port, see Engine::remoteDebuggingPort.
        std::optional<int> remoteDebuggingPort = std::nullopt;

        /// Name of the cache directory. Defaults to the executable name.
        std::optional<std::string> applicationId = std::nullopt;

        /// Wayland app id and X11 WM_CLASS of the windows. Defaults to g_get_prgname(), $FLATPAK_ID or the
        /// executable name, in this order.
        std::optional<std::string> desktopApplicationId = std::nullopt;

        /// Base directory for the engine's caches. Defaults to the user cache directory, can also be set with the
        /// NUI_CEF_CACHE_ROOT environment variable.
        std::optional<std::filesystem::path> cacheRoot = std::nullopt;

        /// Path of the helper executable that runs the engine's child processes. Found automatically if not set.
        std::optional<std::filesystem::path> helperPath = std::nullopt;

        /// Passes the process arguments to Chromium, so switches like --ozone-platform=x11 work on the command line.
        bool passProcessArguments = true;

        SandboxPolicy sandbox = SandboxPolicy::Auto;
    };

    /**
     * @brief Optional handle to the process wide browser engine.
     *
     * Without an Engine object the engine starts with default options at the first Window::run. An Engine sets the
     * options before that and can start the engine early. With the native engines this class does nothing.
     *
     * The CEF engine runs one message loop per process. It ends when the last window has closed and no KeepAlive
     * exists. After that no new window can be created in the process.
     */
    class Engine
    {
      public:
        /**
         * @brief Keeps the message loop running while no window is open, for example for native windows that are not
         * browser windows.
         */
        class KeepAlive
        {
          public:
            KeepAlive();
            ~KeepAlive();
            KeepAlive(KeepAlive const&) = delete;
            KeepAlive& operator=(KeepAlive const&) = delete;
            KeepAlive(KeepAlive&&) = delete;
            KeepAlive& operator=(KeepAlive&&) = delete;
        };

        /**
         * @brief Sets the engine options. Only one Engine may exist, and only before the engine started.
         *
         * @param options Process wide engine settings.
         */
        explicit Engine(EngineOptions options = {});
        ~Engine();
        Engine(Engine const&) = delete;
        Engine& operator=(Engine const&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        /**
         * @brief Starts the engine now instead of at the first Window::run. Must be called on the main thread.
         */
        void start();

        /**
         * @brief Port of the DevTools endpoint, once the engine runs with EngineOptions::remoteDebuggingPort set.
         */
        std::optional<int> remoteDebuggingPort() const;
    };
}
