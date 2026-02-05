#include <nui/frontend/api/drop_effect.hpp>

namespace Nui::WebApi
{
    std::optional<DropEffect> dropEffectFromString(const std::string& effect)
    {
        if (effect == "none")
            return DropEffect::None;
        if (effect == "copy")
            return DropEffect::Copy;
        if (effect == "move")
            return DropEffect::Move;
        if (effect == "link")
            return DropEffect::Link;
        return std::nullopt;
    }
    std::string dropEffectToString(DropEffect effect)
    {
        switch (effect)
        {
            case DropEffect::None:
                return "none";
            case DropEffect::Copy:
                return "copy";
            case DropEffect::Move:
                return "move";
            case DropEffect::Link:
                return "link";
            default:
                return "none";
        }
    }
}