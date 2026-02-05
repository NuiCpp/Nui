#include <algorithm>
#include <nui/screen.hpp>

#include <gdk/gdk.h>

#include <ranges>

namespace Nui
{
    namespace
    {
        Display convertDisplay(auto* monitor, bool isPrimary)
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
                isPrimary,
                model};
        }
    }

    std::vector<Display> Screen::getDisplays()
    {
#if GDK_MAJOR_VERSION >= 4
        auto* display = gdk_display_get_default();
        if (display == nullptr)
            return {};

        auto* monitors = gdk_display_get_monitors(display);
        std::vector<Display> displays;
        displays.reserve(static_cast<std::size_t>(g_list_model_get_n_items(monitors)));
        for (guint i = 0; i != g_list_model_get_n_items(monitors); ++i)
        {
            auto* monitor = GDK_MONITOR(g_list_model_get_item(monitors, i));
            displays.push_back(convertDisplay(monitor, i == 0));
        }
        return displays;
#else
        auto* screen = gdk_screen_get_default();
        auto* display = gdk_screen_get_display(screen);
        int monitorCount = gdk_display_get_n_monitors(display);

        std::vector<Display> displays;
        displays.reserve(static_cast<std::size_t>(monitorCount));
        for (int i = 0; i != monitorCount; ++i)
        {
            auto* monitor = gdk_display_get_monitor(display, i);
            displays.push_back(convertDisplay(monitor, static_cast<bool>(gdk_monitor_is_primary(monitor))));
        }
        return displays;
#endif
    }

    Display Screen::getPrimaryDisplay()
    {
#if GDK_MAJOR_VERSION >= 4
        auto displays = getDisplays();
        if (displays.empty())
            return Display{0, 0, 0, 0, false, "Unknown"};
        return *std::ranges::find_if(displays, [](const Display& display) {
            return display.isPrimary();
        });
#else
        auto* screen = gdk_screen_get_default();
        if (screen == nullptr)
            return Display{0, 0, 0, 0, false, "Unknown"};

        auto* display = gdk_screen_get_display(screen);
        if (display == nullptr)
            return Display{0, 0, 0, 0, false, "Unknown"};

        auto* monitor = gdk_display_get_primary_monitor(display);
        return convertDisplay(monitor, true);
#endif
    }
}