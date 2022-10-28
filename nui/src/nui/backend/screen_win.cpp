#include <nui/screen.hpp>

#include <windows.h>

namespace Nui
{
    std::vector<Display> Screen::getDisplays()
    {
        std::vector<Display> displays;
        EnumDisplayMonitors(
            nullptr,
            nullptr,
            +[](HMONITOR monitor, HDC, LPRECT, LPARAM data) {
                auto displays = reinterpret_cast<std::vector<Display>*>(data);
                MONITORINFOEX info;
                info.cbSize = sizeof(MONITORINFOEX);
                GetMonitorInfo(monitor, &info);
                displays->emplace_back(Display{
                    info.rcMonitor.left,
                    info.rcMonitor.top,
                    info.rcMonitor.right - info.rcMonitor.left,
                    info.rcMonitor.bottom - info.rcMonitor.top,
                    (info.dwFlags & MONITORINFOF_PRIMARY) > 0,
                    info.szDevice});
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&displays));
        return displays;
    }

    Display Screen::getPrimaryDisplay()
    {
        const auto displays = getDisplays();
        for (const auto& display : displays)
        {
            if (display.isPrimary())
                return display;
        }
        throw std::runtime_error("No primary display found");
    }
}