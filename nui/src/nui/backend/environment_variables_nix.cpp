#include <nui/environment_variables.hpp>

#include <cstdio>

extern char** environ;

namespace Nui
{
    std::unordered_map<std::string, std::string> getEnvironmentVariables()
    {
        std::unordered_map<std::string, std::string> result;
        for (char** env = environ; *env != nullptr; env++)
        {
            std::string envString(*env);
            auto pos = envString.find('=');
            if (pos != std::string::npos)
                result[envString.substr(0, pos)] = envString.substr(pos + 1);
            else
                result[envString] = "";
        }
        return result;
    }
}