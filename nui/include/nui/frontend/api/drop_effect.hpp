#pragma once

#include <string>
#include <optional>

namespace Nui::WebApi
{
    enum class DropEffect
    {
        None,
        Copy,
        Move,
        Link
    };

    std::optional<DropEffect> dropEffectFromString(const std::string& effect);
    std::string dropEffectToString(DropEffect effect);
}