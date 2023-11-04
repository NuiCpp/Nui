#include <nui/screen.hpp>

#include <gdk/gdk.h>

namespace Nui
{
    namespace
    {
        Display convertDisplay(auto* monitor)
        {
            if (monitor == nullptr)
                return Display{0, 0, 0, 0, false, "Unknown"};

            GdkRectangle geometry;
            gdk_monitor_get_geometry(monitor, &geometry);
            int factor = gdk_monitor_get_scale_factor(monitor);

            char const* model = gdk_monitor_get_model(monitor);
            if (model == nullptr)
                model = "Unknown";

            return Display{
                geometry.x * factor,
                geometry.y * factor,
                geometry.width * factor,
                geometry.height * factor,
                static_cast<bool>(gdk_monitor_is_primary(monitor)),
                model};
        }
    }

    std::vector<Display> Screen::getDisplays()
    {
        auto* screen = gdk_screen_get_default();
        auto* display = gdk_screen_get_display(screen);
        int monitorCount = gdk_display_get_n_monitors(display);

        std::vector<Display> displays;
        displays.reserve(static_cast<std::size_t>(monitorCount));
        for (int i = 0; i != monitorCount; ++i)
        {
            auto* monitor = gdk_display_get_monitor(display, i);
            displays.push_back(convertDisplay(monitor));
        }
        return displays;
    }

    Display Screen::getPrimaryDisplay()
    {
        auto* screen = gdk_screen_get_default();
        if (screen == nullptr)
            return Display{0, 0, 0, 0, false, "Unknown"};

        auto* display = gdk_screen_get_display(screen);
        if (display == nullptr)
            return Display{0, 0, 0, 0, false, "Unknown"};

        auto* monitor = gdk_display_get_primary_monitor(display);
        return convertDisplay(monitor);
    }
}