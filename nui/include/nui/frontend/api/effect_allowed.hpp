#pragma once

#include <string>
#include <optional>

namespace Nui::WebApi
{
    enum class EffectAllowed
    {
        None,
        Copy,
        CopyLink,
        CopyMove,
        Link,
        LinkMove,
        Move,
        All,
        Uninitialized
    };

    std::optional<EffectAllowed> effectAllowedFromString(const std::string& effect);
    std::string effectAllowedToString(EffectAllowed effect);
}