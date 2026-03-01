#include <nui/screen.hpp>

#include <nui/frontend/rpc_client.hpp>

#include <nui/frontend/val.hpp>

#include <vector>
#include <functional>
#include <utility>

namespace Nui
{
    void Screen::getDisplays(std::function<void(std::vector<Display>&&)> callback)
    {
        RpcClient::getRemoteCallableWithBackChannel(
            "Nui::Screen::getDisplays", [callback = std::move(callback)](Nui::val response) {
                std::vector<Display> result;
                const auto length = response["length"].as<int32_t>();
                result.reserve(length);
                for (int32_t i = 0; i < length; ++i)
                {
                    result.push_back(
                        Display{
                            response[i]["x"].as<int32_t>(),
                            response[i]["y"].as<int32_t>(),
                            response[i]["width"].as<int32_t>(),
                            response[i]["height"].as<int32_t>(),
                            response[i]["isPrimary"].as<bool>(),
                            response[i]["deviceName"].as<std::string>()});
                }
                callback(std::move(result));
            })();
    }
    void Screen::getPrimaryDisplay(std::function<void(Display&&)> callback)
    {
        RpcClient::getRemoteCallableWithBackChannel(
            "Nui::Screen::getPrimaryDisplay", [callback = std::move(callback)](Nui::val response) {
                callback(
                    Display{
                        response["x"].as<int32_t>(),
                        response["y"].as<int32_t>(),
                        response["width"].as<int32_t>(),
                        response["height"].as<int32_t>(),
                        response["isPrimary"].as<bool>(),
                        response["deviceName"].as<std::string>()});
            })();
    }
}