#include <nui/backend/webkit_gtk_rendering.hpp>

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(NUI_ENGINE_CEF)
#    include <fmt/format.h>

#    include <unistd.h>

#    include <algorithm>
#    include <array>
#    include <cstdlib>
#    include <filesystem>
#    include <fstream>
#    include <functional>
#    include <optional>
#    include <string_view>
#    include <system_error>
#    include <vector>
#endif

namespace Nui
{
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(NUI_ENGINE_CEF)
    namespace
    {
        constexpr std::array<char const*, 4> renderingVariables{
            "WEBKIT_DISABLE_DMABUF_RENDERER",
            "WEBKIT_DMABUF_RENDERER_FORCE_SHM",
            "WEBKIT_DISABLE_COMPOSITING_MODE",
            "__NV_DISABLE_EXPLICIT_SYNC",
        };
        constexpr std::array<std::string_view, 3> verifiedDrivers{"amdgpu", "i915", "xe"};
        constexpr std::string_view nvidiaVendorId = "0x10de";

        struct SystemPaths
        {
            std::filesystem::path sysfsRoot;
            std::filesystem::path deviceRoot;
        };

        struct GraphicsCard
        {
            std::string name;
            std::string driver;
            std::vector<std::string> renderNodes;
        };

        std::string_view environmentValue(char const* name)
        {
            char const* const value = std::getenv(name);
            return value == nullptr ? std::string_view{} : std::string_view{value};
        }

        bool isTruthy(std::string_view value)
        {
            return !value.empty() && value != "0" && value != "false";
        }

        std::optional<std::string> readFirstLine(std::filesystem::path const& path)
        {
            std::ifstream file{path};
            if (!file)
                return std::nullopt;
            std::string line;
            if (!std::getline(file, line))
                return std::nullopt;
            return line;
        }

        void forEachDirectoryEntry(
            std::filesystem::path const& directory,
            std::function<void(std::filesystem::path const&)> const& visitor)
        {
            std::error_code error;
            for (auto iterator = std::filesystem::directory_iterator{directory, error};
                 !error && iterator != std::filesystem::directory_iterator{};
                 iterator.increment(error))
            {
                visitor(iterator->path());
            }
        }

        bool hasDisplaySession()
        {
            // GDK_BACKEND may name several backends, GTK takes the first it can open.
            std::string_view backends = environmentValue("GDK_BACKEND");
            while (!backends.empty())
            {
                const auto separator = backends.find(',');
                const auto backend = backends.substr(0, separator);
                if (backend == "x11" || backend == "wayland")
                    return true;
                backends = separator == std::string_view::npos ? std::string_view{} : backends.substr(separator + 1);
            }

            const auto sessionType = environmentValue("XDG_SESSION_TYPE");
            if (sessionType == "x11" || sessionType == "wayland")
                return true;

            // Flatpak does not pass XDG_SESSION_TYPE into the sandbox.
            return !environmentValue("WAYLAND_DISPLAY").empty() || !environmentValue("DISPLAY").empty();
        }

        /**
         * @brief Looks for NVIDIA display devices and display devices that cannot be identified.
         *
         * Also catches the proprietary driver without nvidia-drm modesetting, which creates no DRM card.
         *
         * @param paths Roots of sysfs and /dev.
         * @return The reason to disable the renderer, if any.
         */
        std::optional<std::string> findUnsafeDisplayDevice(SystemPaths const& paths)
        {
            std::error_code error;
            if (std::filesystem::exists(paths.sysfsRoot / "module" / "nvidia", error))
                return "the NVIDIA kernel module is loaded";

            std::optional<std::string> reason;
            forEachDirectoryEntry(paths.sysfsRoot / "bus" / "pci" / "devices", [&](auto const& device) {
                if (reason)
                    return;
                const auto deviceName = device.filename().string();
                const auto deviceClass = readFirstLine(device / "class");
                if (!deviceClass)
                {
                    reason = fmt::format("the class of PCI device {} is unreadable", deviceName);
                    return;
                }
                if (!deviceClass->starts_with("0x03"))
                    return;

                const auto vendor = readFirstLine(device / "vendor");
                if (!vendor)
                    reason = fmt::format("the vendor of display device {} is unreadable", deviceName);
                else if (*vendor == nvidiaVendorId)
                    reason = fmt::format("{} is an NVIDIA display device", deviceName);
            });
            return reason;
        }

        std::vector<GraphicsCard> enumerateGraphicsCards(SystemPaths const& paths)
        {
            std::vector<GraphicsCard> cards;
            forEachDirectoryEntry(paths.sysfsRoot / "class" / "drm", [&](auto const& cardPath) {
                auto name = cardPath.filename().string();
                // Connectors are listed as cardN-<connector>.
                if (!name.starts_with("card") || name.find('-') != std::string::npos)
                    return;

                GraphicsCard card{.name = std::move(name), .driver = {}, .renderNodes = {}};
                std::error_code error;
                const auto driverLink = std::filesystem::read_symlink(cardPath / "device" / "driver", error);
                if (!error)
                    card.driver = driverLink.filename().string();

                forEachDirectoryEntry(cardPath / "device" / "drm", [&](auto const& node) {
                    auto nodeName = node.filename().string();
                    if (nodeName.starts_with("renderD"))
                        card.renderNodes.push_back(std::move(nodeName));
                });
                cards.push_back(std::move(card));
            });
            return cards;
        }

        WebKitGtkRenderingConfiguration disabled(std::string_view reason)
        {
            return {.decision = DmabufRendererDecision::Disabled, .reason = fmt::format("disabled, {}", reason)};
        }

        WebKitGtkRenderingConfiguration decideDmabufRenderer(SystemPaths const& paths)
        {
            for (auto const* variable : renderingVariables)
            {
                if (std::getenv(variable) != nullptr)
                    return {
                        .decision = DmabufRendererDecision::KeptEnvironment,
                        .reason = fmt::format("left to the environment, {} is set", variable),
                    };
            }

            if (isTruthy(environmentValue("LIBGL_ALWAYS_SOFTWARE")))
                return disabled("software rendering is forced by LIBGL_ALWAYS_SOFTWARE");

            if (!hasDisplaySession())
                return disabled("the session type is neither X11 nor Wayland");

            if (const auto unsafeDisplayDevice = findUnsafeDisplayDevice(paths))
                return disabled(*unsafeDisplayDevice);

            const auto cards = enumerateGraphicsCards(paths);
            if (cards.empty())
                return disabled("no DRM graphics card was found");

            bool renderNodeAccessible = false;
            for (auto const& card : cards)
            {
                if (card.driver.empty())
                    return disabled(fmt::format("the driver of {} is unknown", card.name));
                if (std::ranges::find(verifiedDrivers, card.driver) == verifiedDrivers.end())
                    return disabled(fmt::format("{} uses the unverified driver {}", card.name, card.driver));

                renderNodeAccessible =
                    renderNodeAccessible || std::ranges::any_of(card.renderNodes, [&](auto const& node) {
                        const auto nodePath = paths.deviceRoot / "dri" / node;
                        return ::access(nodePath.c_str(), R_OK | W_OK) == 0;
                    });
            }
            if (!renderNodeAccessible)
                return disabled("no render node is accessible");

            return {
                .decision = DmabufRendererDecision::Enabled,
                .reason = "enabled, all graphics cards use verified drivers",
            };
        }
    }

    WebKitGtkRenderingConfiguration configureWebKitGtkRendering()
    {
        const auto configuration = decideDmabufRenderer({.sysfsRoot = "/sys", .deviceRoot = "/dev"});
        if (configuration.decision == DmabufRendererDecision::Disabled)
            ::setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", 1);
        return configuration;
    }
#else
    WebKitGtkRenderingConfiguration configureWebKitGtkRendering()
    {
        return {.decision = DmabufRendererDecision::NotApplicable, .reason = "not a WebKitGTK platform"};
    }
#endif
}
