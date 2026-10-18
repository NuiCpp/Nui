#pragma once

#include <filesystem>
#include <string>

namespace Nui::Detail::Cef
{
    struct SandboxDecision
    {
        /// Start the child processes with --no-sandbox.
        bool disable{false};
        /// Why the sandbox is off, empty if it is on.
        std::string reason{};
    };

    /**
     * @brief Whether Chromium's Linux sandbox can work: unprivileged user namespaces (the namespace sandbox), a usable
     * setuid chrome-sandbox next to libcef, or zypak inside Flatpak. Mirrors Chromium's own checks, so Chromium never
     * stops with "No usable sandbox".
     *
     * @param libcefDirectory Directory of the loaded libcef.so.
     */
    SandboxDecision probeSandbox(std::filesystem::path const& libcefDirectory);
}
