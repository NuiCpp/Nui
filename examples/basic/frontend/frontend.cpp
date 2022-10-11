#include <nui/core.hpp>
#include <nui/frontend/dom/dom.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <emscripten/val.h>

void frontendMain()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using Nui::Elements::div; // there is a global div (division).

    // clang-format off
    const auto page = body{}(
        div{}("Hello World")
    );
    //  clang-format on

    thread_local Dom::Dom dom;
    dom.setBody(page);
}

EMSCRIPTEN_BINDINGS(mymod)
{
    emscripten::function("main", &frontendMain);
}

#include <nui/bindings.hpp>