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
#include <nui/frontend/attributes/mouse_events.hpp>
#include <nui/frontend/attributes/reference.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/frontend/attributes/type.hpp>

#include <emscripten.h>

namespace Nui::Components
{
    // #####################################################################################################################
    DialogController::DialogController(ConstructionArgs&& args)
        : isOpen_{false}
        , element_{}
        , className_{std::move(args.className)}
        , title_{std::move(args.title)}
        , body_{std::move(args.body)}
        , buttonClassName_{std::move(args.buttonClassName)}
        , buttonConfiguration_{std::move(args.buttonConfiguration)}
        , onButtonClicked_{std::move(args.onButtonClicked)}
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
        className_ = className;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setButtonClassName(std::string const& className)
    {
        buttonClassName_ = className;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setTitle(std::string const& title)
    {
        title_ = title;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setBody(std::string const& body)
    {
        body_ = body;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setButtonConfiguration(ButtonConfiguration buttons)
    {
        buttonConfiguration_ = buttons;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void DialogController::setOnButtonClicked(std::function<void(Button)> const& onButtonClicked)
    {
        onButtonClicked_ = onButtonClicked;
    }
    //---------------------------------------------------------------------------------------------------------------------
    Nui::ElementRenderer Dialog(DialogController& controller)
    {
        using namespace Nui::Attributes;
        using namespace Nui::Elements;

        // clang-format off
        return dialog{
            class_ = controller.className_,
            reference = [&controller](auto&& element){
                controller.element_ = std::static_pointer_cast<Dom::Element>(element.lock());
            }
        }(
            form{
                method = "dialog"
            }(
                h1{}(
                    controller.title_
                ),
                p{}(
                    controller.body_
                ),
                menu{}(
                    observe(controller.buttonConfiguration_),
                    [&controller, &conf = controller.buttonConfiguration_]() -> Nui::ElementRenderer {
                        switch (conf.value()) {
                            case(DialogController::ButtonConfiguration::Ok):
                            {
                                return button{
                                    class_ = controller.buttonClassName_,
                                    type = "submit",
                                    onClick = [&controller](){
                                        controller.isOpen_ = false;
                                        controller.onButtonClicked_(DialogController::Button::Ok);
                                    }
                                }("Ok");
                            }
                            case(DialogController::ButtonConfiguration::OkCancel):
                            {
                                return fragment(
                                    button{
                                        class_ = controller.buttonClassName_,
                                        type = "submit",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.onButtonClicked_(DialogController::Button::Ok);
                                        }
                                    }("Ok"),
                                    button{
                                        class_ = controller.buttonClassName_,
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.onButtonClicked_(DialogController::Button::Cancel);
                                        }
                                    }("Cancel")
                                );
                            }
                            case(DialogController::ButtonConfiguration::YesNo):
                            {
                                return fragment(
                                    button{
                                        class_ = controller.buttonClassName_,
                                        type = "submit",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.onButtonClicked_(DialogController::Button::Yes);
                                        }
                                    }("Yes"),
                                    button{
                                        class_ = controller.buttonClassName_,
                                        type = "cancel",
                                        onClick = [&controller](){
                                            controller.isOpen_ = false;
                                            controller.onButtonClicked_(DialogController::Button::No);
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