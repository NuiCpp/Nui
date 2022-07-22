#pragma once

namespace Nui::Elements
{
    class HtmlElement
    {
    public:
        template <typename... Attributes>
        HtmlElement(Attributes&&... attributes)
        {
        }

    private:
        std::vector <std::string> attributes;
    };
}