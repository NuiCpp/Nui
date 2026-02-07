#include <nui/frontend/dom/dom.hpp>

#include <nui/frontend/val.hpp>

namespace Nui::Dom
{
    // #####################################################################################################################
    Dom::Dom(std::optional<Nui::val> contentRoot)
        : root_{std::make_shared<Element>(contentRoot ? *contentRoot : Nui::val::global("document")["body"])}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Element& Dom::root()
    {
        return *root_;
    }
    // #####################################################################################################################
}