#pragma once

#include <nui/core.hpp>

#include <unordered_map>
#include <string>
#ifdef NUI_FRONTEND
#    include <functional>
#endif

namespace Nui
{
#ifdef NUI_BACKEND
    std::unordered_map<std::string, std::string> getEnvironmentVariables();
#else
    void getEnvironmentVariables(std::function<void(std::unordered_map<std::string, std::string>&&)> callback);
#endif
}