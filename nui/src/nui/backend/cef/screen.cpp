#include "engine_state.hpp"

#include <nui/screen.hpp>

#include <include/views/cef_display.h>

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <vector>

namespace Nui
{
    namespace
    {
        Display convertDisplay(CefRefPtr<CefDisplay> const& display, bool isPrimary)
        {
            const auto bounds = display->GetBounds();
            const auto scale = display->GetDeviceScaleFactor();
            return Display{
                static_cast<int32_t>(static_cast<float>(bounds.x) * scale),
                static_cast<int32_t>(static_cast<float>(bounds.y) * scale),
                static_cast<int32_t>(static_cast<float>(bounds.width) * scale),
                static_cast<int32_t>(static_cast<float>(bounds.height) * scale),
                isPrimary,
                std::to_string(display->GetID())};
        }

        std::vector<Display> collectDisplays()
        {
            const auto primary = CefDisplay::GetPrimaryDisplay();
            std::vector<CefRefPtr<CefDisplay>> cefDisplays;
            CefDisplay::GetAllDisplays(cefDisplays);
            std::vector<Display> displays;
            displays.reserve(cefDisplays.size());
            for (auto const& display : cefDisplays)
                displays.push_back(convertDisplay(display, primary && display->GetID() == primary->GetID()));
            return displays;
        }

        // CefDisplay needs the started engine and its UI thread.
        std::vector<Display> displaysOnUiThread()
        {
            auto& engine = Detail::Cef::EngineState::instance();
            try
            {
                if (!engine.started() && Detail::Cef::isMainThread())
                    engine.start();
            }
            catch (std::exception const& exception)
            {
                std::cerr << "Nui: no displays, the engine did not start: " << exception.what() << std::endl;
                return {};
            }
            if (!engine.started())
                return {};
            if (engine.onUiThread())
                return collectDisplays();

            auto promise = std::make_shared<std::promise<std::vector<Display>>>();
            auto result = promise->get_future();
            engine.postToUi([promise]() {
                promise->set_value(collectDisplays());
            });
            if (result.wait_for(std::chrono::seconds{5}) != std::future_status::ready)
                return {};
            return result.get();
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::vector<Display> Screen::getDisplays()
    {
        return displaysOnUiThread();
    }
    //---------------------------------------------------------------------------------------------------------------------
    Display Screen::getPrimaryDisplay()
    {
        for (auto const& display : displaysOnUiThread())
        {
            if (display.isPrimary())
                return display;
        }
        return Display{0, 0, 0, 0, false, "Unknown"};
    }
}
