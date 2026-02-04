#include <nui/frontend/api/effect_allowed.hpp>

namespace Nui::WebApi
{
    std::optional<EffectAllowed> effectAllowedFromString(const std::string& effect)
    {
        if (effect == "none")
            return EffectAllowed::None;
        if (effect == "copy")
            return EffectAllowed::Copy;
        if (effect == "copyLink")
            return EffectAllowed::CopyLink;
        if (effect == "copyMove")
            return EffectAllowed::CopyMove;
        if (effect == "link")
            return EffectAllowed::Link;
        if (effect == "linkMove")
            return EffectAllowed::LinkMove;
        if (effect == "move")
            return EffectAllowed::Move;
        if (effect == "all")
            return EffectAllowed::All;
        if (effect == "uninitialized")
            return EffectAllowed::Uninitialized;
        return std::nullopt;
    }
    std::string effectAllowedToString(EffectAllowed effect)
    {
        switch (effect)
        {
            case EffectAllowed::None:
                return "none";
            case EffectAllowed::Copy:
                return "copy";
            case EffectAllowed::CopyLink:
                return "copyLink";
            case EffectAllowed::CopyMove:
                return "copyMove";
            case EffectAllowed::Link:
                return "link";
            case EffectAllowed::LinkMove:
                return "linkMove";
            case EffectAllowed::Move:
                return "move";
            case EffectAllowed::All:
                return "all";
            case EffectAllowed::Uninitialized:
                return "uninitialized";
            default:
                return "none";
        }
    }
}