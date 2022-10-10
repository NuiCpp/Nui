#include <nui/frontend/components/dialog.hpp>

#include <nui/frontend/elements/dialog.hpp>
#include <nui/frontend/elements/form.hpp>
#include <nui/frontend/elements/p.hpp>
#include <nui/frontend/elements/menu.hpp>
#include <nui/frontend/elements/heading.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/fragment.hpp>
#include <nui/frontend/attributes/open.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/method.hpp>
#include <nui/frontend/attributes/type.hpp>
#include <nui/frontend/attributes/mouse_events.hpp>
#include <nui/frontend/attributes/style.hpp>

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
    bool DialogController::isOpen() const
    {
        return isOpen_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    Nui::ElementRenderer Dialog(DialogController& controller)
    {
        using namespace Nui::Attributes;
        using namespace Nui::Elements;

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
                                        controller.args_.onButtonClicked_(DialogController::Button::Ok);
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
                                            controller.args_.onButtonClicked_(DialogController::Button::Ok);
                                        }
                                    }("Ok"),
                                    button{
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.args_.onButtonClicked_(DialogController::Button::Cancel);
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
                                            controller.args_.onButtonClicked_(DialogController::Button::Yes);
                                        }
                                    }("Yes"),
                                    button{
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.args_.onButtonClicked_(DialogController::Button::No);
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