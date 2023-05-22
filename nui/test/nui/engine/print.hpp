#pragma once

#include <iostream>

namespace Nui::Tests::Engine
{
    inline void printIndent(int indent)
    {
        for (int i = 0; i < indent; ++i)
            std::cout << "  ";
    }
}