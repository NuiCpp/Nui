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
    // #####################################################################################################################
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
    void DialogController::hide()
    {
        isOpen_ = false;
        if (auto element = element_.lock())
            element->val().call<void>("close");
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool DialogController::isOpen() const
    {
        return isOpen_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setClassName(std::string const& className)
    {
        args_.className = className;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setButtonClassName(std::string const& className)
    {
        args_.buttonClassName = className;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setTitle(std::string const& title)
    {
        args_.title = title;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setBody(std::string const& body)
    {
        args_.body = body;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setButtonConfiguration(ButtonConfiguration buttons)
    {
        args_.buttonConfiguration = buttons;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setOnButtonClicked(std::function<void(Button)> const& onButtonClicked)
    {
        args_.onButtonClicked = onButtonClicked;
    }
    //---------------------------------------------------------------------------------------------------------------------
    Nui::ElementRenderer Dialog(DialogController& controller)
    {
        using namespace Nui::Attributes;
        using namespace Nui::Elements;

        // clang-format off
        return dialog{
            class_ = controller.args_.className
        }(
            Dom::reference([&controller](auto element){
                controller.element_ = std::static_pointer_cast<Dom::Element>(element.lock());
            }),
            form{
                method = "dialog"
            }(
                h1{}(
                    controller.args_.title
                ),
                p{}(
                    controller.args_.body
                ),
                menu{}(
                    observe(controller.args_.buttonConfiguration),
                    [&controller, &conf = controller.args_.buttonConfiguration]() -> Nui::ElementRenderer {
                        switch (conf.value()) {
                            case(DialogController::ButtonConfiguration::Ok):
                            {
                                return button{
                                    class_ = controller.args_.buttonClassName,
                                    type = "submit",
                                    onClick = [&controller](){
                                        controller.isOpen_ = false;
                                        controller.args_.onButtonClicked(DialogController::Button::Ok);
                                    }
                                }("Ok");
                            }
                            case(DialogController::ButtonConfiguration::OkCancel):
                            {
                                return fragment(
                                    button{
                                        class_ = controller.args_.buttonClassName,
                                        type = "submit",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.args_.onButtonClicked(DialogController::Button::Ok);
                                        }
                                    }("Ok"),
                                    button{
                                        class_ = controller.args_.buttonClassName,
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.args_.onButtonClicked(DialogController::Button::Cancel);
                                        }
                                    }("Cancel")
                                );
                            }
                            case(DialogController::ButtonConfiguration::YesNo):
                            {
                                return fragment(
                                    button{
                                        class_ = controller.args_.buttonClassName,
                                        type = "submit",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.args_.onButtonClicked(DialogController::Button::Yes);
                                        }
                                    }("Yes"),
                                    button{
                                        class_ = controller.args_.buttonClassName,
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.args_.onButtonClicked(DialogController::Button::No);
                                        }
                                    }("No")
                                );
                            }
                            case(DialogController::ButtonConfiguration::None):
                            {
                                return fragment();
                            }
                        }
                    }
                )
            )
        );
        // clang-format on
    }
    // #####################################################################################################################
}