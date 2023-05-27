#pragma once

#include <nui/frontend/val.hpp>

namespace Nui::Tests::Engine
{
    class Document
    {
      public:
        Document();

        Nui::val document();
    };
}