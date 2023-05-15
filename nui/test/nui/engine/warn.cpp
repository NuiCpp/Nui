#include "warn.hpp"

#include <iostream>

namespace Nui::Tests::Engine
{
    void warn(std::string_view message)
    {
        std::cout << "Warning: " << message << std::endl;
    }
}