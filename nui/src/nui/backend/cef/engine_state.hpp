#pragma once

#include "window_shared.hpp"

#include <nui/engine.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Nui::Detail::Cef
{
    /**
     * @brief Process wide state of the CEF engine: options, start and shutdown, the message loop and the browsers.
     *
     * Methods without a thread note must be called on the main thread, which is also CEF's UI thread.
     */
    class EngineState
    {
      public:
        static EngineState& instance();

        /**
         * @brief Sets the options of an explicit Engine. Throws if the engine started or another Engine exists.
         */
        void attachEngine(EngineOptions options);
        void detachEngine();

        /**
         * @brief Adds the scheme names and https authority of a window constructed before the engine starts, or checks
         * them against the registered ones afterwards. Throws for names that can no longer be registered.
         */
        void registerWindowRequirements(
            std::vector<std::string> const& schemes,
            std::string const& htmlAuthority,
            bool debug,
            std::optional<std::string> const& browserArguments);

        /**
         * @brief Starts the engine if it did not start yet. Throws if it cannot start, also on later calls.
         */
        void start();

        bool started() const;
        bool loopRunning() const;
        bool finished() const;

        /**
         * @brief Runs the message loop until the last window closed and no KeepAlive exists, then shuts down.
         */
        void runLoop();

        /**
         * @brief Shuts down an engine that started but never ran its loop, from the process exit hook.
         */
        void shutdownAtExit();

        /**
         * @brief A window that will get a browser counts as open from here until its browser closed.
         *
         * @param createBrowser Creates the browser, called on the UI thread when the loop starts or right away
         * (posted) when it runs.
         */
        void windowOpened(std::function<void()> createBrowser);
        void windowClosed();

        /**
         * @brief Closes every window, for the platform's quit (macOS terminate:).
         */
        void closeAllWindows();

        void addKeepAlive();
        void removeKeepAlive();

        /// Any thread.
        void registerBrowser(int browserId, std::shared_ptr<WindowShared> window);
        /// Any thread.
        void unregisterBrowser(int browserId);
        /// Any thread. The window of a browser, or for requests without a browser the only window with the scheme.
        std::shared_ptr<WindowShared> windowOf(int browserId, std::string const& scheme) const;

        /// Any thread. Runs the function on the UI thread, before the start it is queued until the engine started.
        void postToUi(std::function<void()> function);

        /// Any thread. True on the UI thread of a started engine, or on the main thread before the start.
        bool onUiThread() const;

        /**
         * @brief Any thread. Runs a custom scheme body reader call on the reader pool.
         *
         * @param work Calls the application's reader, may block. Returns a byte count or a net error.
         * @param complete Hands the result of work to CEF. Not called for readers that only return after CefShutdown,
         * its CEF objects are leaked then.
         * @return False when the pool is gone (the engine shuts down), neither function runs then.
         */
        bool postReader(std::function<int()> work, std::function<void(int)> complete);

        /// Any thread. True when the shutdown stopped waiting for body readers that still block.
        bool readersAbandoned() const;

        std::optional<int> remoteDebuggingPort() const;
        std::string const& desktopApplicationId() const;
        std::vector<std::string> const& customSchemes() const;

        /// Any thread. Every https authority a window served setHtml from so far.
        std::vector<std::string> htmlAuthorities() const;

        /// Any thread. True for a registered custom scheme, or https with a setHtml authority.
        bool isAppOrigin(std::string const& scheme, std::string const& host) const;

        /// Registers the https factory serving setHtml for an authority, if it is not registered yet.
        void ensureHtmlAuthority(std::string const& authority);

        EngineState(EngineState const&) = delete;
        EngineState& operator=(EngineState const&) = delete;
        EngineState(EngineState&&) = delete;
        EngineState& operator=(EngineState&&) = delete;

      private:
        EngineState();
        ~EngineState();

        void shutdown();
        void quitIfIdle();

        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };

    /**
     * @brief True on the process's main thread.
     */
    bool isMainThread();
}
