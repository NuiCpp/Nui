#pragma once

#include <string>

namespace Nui::Components
{
    template <typename T>
    struct SelectOptions
    {
        T value;
        std::string label;
    };
}