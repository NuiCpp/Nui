#include <nui/environment_variables.hpp>

#include <nui/frontend/rpc_client.hpp>

namespace Nui
{
    void getEnvironmentVariables(std::function<void(std::unordered_map<std::string, std::string>&&)> callback)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::getEnvironmentVariables", std::move(callback))();
    }
}