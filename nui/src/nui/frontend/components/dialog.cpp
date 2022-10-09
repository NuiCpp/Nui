#include <nui/components/dialog.hpp>

#include <nui/elements/dialog.hpp>
#include <nui/elements/form.hpp>
#include <nui/elements/p.hpp>
#include <nui/elements/menu.hpp>
#include <nui/elements/head.hpp>
#include <nui/elements/button.hpp>
#include <nui/elements/fragment.hpp>
#include <nui/attributes/open.hpp>
#include <nui/attributes/class.hpp>
#include <nui/attributes/method.hpp>
#include <nui/attributes/type.hpp>
#include <nui/attributes/mouse_events.hpp>
#include <nui/attributes/style.hpp>

#include <emscripten.h>

namespace Nui::Components
{
    //#####################################################################################################################
    DialogController::DialogController(ConstructionArgs&& args)
        : isOpen_{false}
        , element_{}
        , args_{std::move(args)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::showModal()
    {
        isOpen_ = true;
        if (auto element = element_.lock())
            element->val().call<void>("showModal");
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
        return dialog{
            class_ = controller.args_.className_
        }(
            Dom::reference([&controller](auto element){
                controller.element_ = std::static_pointer_cast<Dom::Element>(element.lock());
            }),
            form{
                method = "dialog"
            }(
                h1{}(
                    controller.args_.titel_
                ),
                p{}(
                    controller.args_.body_
                ),
                menu{}(
                    observe(controller.args_.buttonConfiguration_),
                    [&controller, &conf = controller.args_.buttonConfiguration_]() -> Nui::ElementRenderer {
                        switch (conf.value()) {
                            case(DialogController::ButtonConfiguration::Ok):
                            {
                                return button{
                                    type = "submit",
                                    onClick = [&controller](){
                                        controller.isOpen_ = false;
                                    }
                                }("Ok");
                            }
                            case(DialogController::ButtonConfiguration::OkCancel):
                            {
                                return fragment(
                                    button{
                                        type = "submit",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                        }
                                    }("Ok"),
                                    button{
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                        }
                                    }("Cancel")
                                );
                            }
                            case(DialogController::ButtonConfiguration::YesNo):
                            {
                                return fragment(
                                    button{
                                        type = "submit",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                        }
                                    }("Yes"),
                                    button{
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                        }
                                    }("No")
                                );
                            }
                        }
                    }
                )
            )
        );
        // clang-format on
    }
    //#####################################################################################################################
}