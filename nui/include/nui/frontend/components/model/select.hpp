#pragma once

#include <string>

namespace Nui::Components
{
    template <typename T>
    struct SelectOptions
    {
        std::string label;
        T value;
    };
}