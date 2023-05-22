#pragma once

#include <emscripten/val.h>

namespace Nui::Tests::Engine
{
    class Document
    {
      public:
        Document();

        emscripten::val document();
    };
}