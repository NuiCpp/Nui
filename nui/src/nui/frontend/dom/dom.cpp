#include <nui/frontend/dom/dom.hpp>
#include <nui/frontend/utility/inferance_helper.hpp>
#include <nui/frontend/elements/div.hpp>

#include <emscripten/val.h>

namespace Nui::Dom
{
    //#####################################################################################################################
    Dom::Dom()
        : root_{std::make_shared<Element>(emscripten::val::global("document")["body"])}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Element& Dom::root()
    {
        return *root_;
    }
    //#####################################################################################################################
}