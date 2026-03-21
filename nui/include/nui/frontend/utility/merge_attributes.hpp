#pragma once

#include <nui/frontend/attributes/impl/attribute.hpp>

#include <vector>
#include <iterator>

namespace Nui
{
    inline std::vector<Nui::Attribute>
    mergeAttributes(std::vector<Nui::Attribute>&& base, std::vector<Nui::Attribute>&& additional)
    {
        base.reserve(base.size() + additional.size());
        if (base.size() > additional.size())
        {
            base.insert(
                base.end(), std::make_move_iterator(additional.begin()), std::make_move_iterator(additional.end()));
            return base;
        }
        additional.insert(additional.end(), std::make_move_iterator(base.begin()), std::make_move_iterator(base.end()));
        return additional;
    }
}