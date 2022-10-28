#include "environment_variables.hpp"

#include <nui/environment_variables.hpp>

namespace Nui
{
    void registerEnvironmentVariables(Nui::RpcHub const& hub)
    {
        hub.registerFunction("Nui::getEnvironmentVariables", [&hub](std::string const& responseId) {
            hub.callRemote(responseId, getEnvironmentVariables());
        });
    }
}