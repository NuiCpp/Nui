#include "screen.hpp"

#include <nui/screen.hpp>
#include <nlohmann/json.hpp>

namespace Nui
{
    void registerScreen(Nui::RpcHub const& hub)
    {
        hub.registerFunction("Nui::Screen::getDisplays", [&hub](std::string const& responseId) {
            const auto displays = Screen::getDisplays();
            auto response = nlohmann::json::array();
            for (auto const& display : displays)
            {
                auto displayJson = nlohmann::json::object();
                displayJson["x"] = display.x();
                displayJson["y"] = display.y();
                displayJson["width"] = display.width();
                displayJson["height"] = display.height();
                displayJson["isPrimary"] = display.isPrimary();
                displayJson["deviceName"] = display.deviceName();
                response.push_back(displayJson);
            }
            hub.callRemote(responseId, response);
        });

        hub.registerFunction("Nui::Screen::getPrimaryDisplay", [&hub](std::string const& responseId) {
            const auto display = Screen::getPrimaryDisplay();
            auto response = nlohmann::json::object();
            response["x"] = display.x();
            response["y"] = display.y();
            response["width"] = display.width();
            response["height"] = display.height();
            response["isPrimary"] = display.isPrimary();
            response["deviceName"] = display.deviceName();
            hub.callRemote(responseId, response);
        });
    }
}