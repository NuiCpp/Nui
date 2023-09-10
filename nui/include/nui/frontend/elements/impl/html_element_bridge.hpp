#pragma once

#include <nui/frontend/val.hpp>

namespace Nui
{
    namespace Dom
    {
        class ChildlessElement;
    }
    class HtmlElement;

    struct HtmlElementBridge
    {
        Nui::val (*createElement)(HtmlElement const& element);
    };
}