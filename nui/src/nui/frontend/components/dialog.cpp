#include <nui/components/dialog.hpp>

#include <nui/elements/dialog.hpp>
#include <nui/elements/form.hpp>
#include <nui/elements/p.hpp>
#include <nui/elements/menu.hpp>
#include <nui/elements/head.hpp>
#include <nui/elements/button.hpp>
#include <nui/attributes/open.hpp>
#include <nui/attributes/method.hpp>
#include <nui/attributes/type.hpp>
#include <nui/attributes/mouse_events.hpp>
#include <nui/attributes/style.hpp>

#include <emscripten.h>

namespace Nui::Components
{
    //#####################################################################################################################
    DialogController::DialogController()
        : isOpen_{false}
        , element_{}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::showModal()
    {
        isOpen_ = true;
        if (auto element = element_.lock())
        {
            emscripten::val::global("console").call<void>("log", element->val());
            element->val().call<void>("showModal");
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::show()
    {
        isOpen_ = true;
        if (auto element = element_.lock())
            element->val().call<void>("show");
    }
    //---------------------------------------------------------------------------------------------------------------------
    Nui::ElementRenderer Dialog(DialogController& controller)
    {
        using namespace Nui::Attributes;
        // clang-format off
        return dialog{}(
            Dom::reference([&controller](auto element){
                controller.element_ = std::static_pointer_cast<Dom::Element>(element.lock());
            }),
            form{
                method = "dialog",
                style = "background-color: red"
            }(
                h1{}("Dialog"),
                p{}("This is a dialog"),
                menu{}(
                    button{
                        type = "submit",
                        onClick = [&controller](auto const&){
                            controller.isOpen_ = false;
                        }
                    }("OK"), 
                    button{
                        type = "cancel",
                        onClick = [&controller](auto const&){
                            controller.isOpen_ = false;
                        }
                    }("Cancel")
                )
            )
        );
        // clang-format on
    }
    //#####################################################################################################################
}