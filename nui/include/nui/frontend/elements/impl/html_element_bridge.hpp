#pragma once

namespace Nui
{
    namespace Dom
    {
        class ChildlessElement;
    }
    class HtmlElement;

    struct HtmlElementBridge
    {
        Dom::ChildlessElement (*createElement)(HtmlElement const& element);
    };
}