#pragma once

#include "gate.hpp"

#include <nui/window.hpp>

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace Nui::Detail::Cef
{
    /**
     * @brief Part of a window that engine threads (scheme handlers, IO) use. Calls hold a GateEntry and a shared_ptr.
     */
    struct WindowShared
    {
        Gate gate{};

        /// Custom schemes of the window by name, immutable after construction.
        std::unordered_map<std::string, std::shared_ptr<CustomScheme const>> schemes{};

        std::shared_mutex htmlGuard{};
        /// Host of the https origin that setHtml serves from.
        std::string htmlAuthority{};
        std::shared_ptr<std::string const> html{};

        std::shared_mutex folderMappingGuard{};
        std::unordered_map<std::string, std::filesystem::path> folderMappings{};

        /// Closes the window, set by the window. Called on the UI thread.
        std::function<void()> requestClose{};
    };
}
