#include <nui/environment_variables.hpp>
#include <nui/utility/scope_exit.hpp>

#include <cstring>

#include <windows.h>

namespace Nui
{
    std::unordered_map<std::string, std::string> getEnvironmentVariables()
    {
        auto* envStrings = GetEnvironmentStrings();
        if (envStrings == nullptr)
            return {};
        auto remover = ScopeExit{[&envStrings]() {
            FreeEnvironmentStrings(envStrings);
        }};
        // var1=value1\0var2=value2\0\0
        std::unordered_map<std::string, std::string> result;
        for (auto* env = envStrings; *env != '\0'; env += strlen(env) + 1)
        {
            std::string envString(env);
            auto pos = envString.find('=');
            if (pos != std::string::npos)
                result[envString.substr(0, pos)] = envString.substr(pos + 1);
            else
                result[envString] = "";
        }
        return result;
    }
}