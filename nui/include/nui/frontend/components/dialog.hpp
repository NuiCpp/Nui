#pragma once

#include <nui/frontend/generator_typedefs.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/dom/element.hpp>

#include <functional>

namespace Nui::Components
{
    /**
     * @brief Examplary dialog class for the most simple kinds of dialogs.
     */
    class DialogController
    {
      public:
        enum class Button
        {
            Ok,
            Cancel,
            Yes,
            No,
        };
        enum class ButtonConfiguration
        {
            Ok,
            OkCancel,
            YesNo,
            None
        };

        struct ConstructionArgs
        {
            Observed<std::optional<std::string>> className;
            Observed<std::string> title;
            Observed<std::string> body;
            Observed<std::string> buttonClassName;
            Observed<ButtonConfiguration> buttonConfiguration;
            std::function<void(Button)> onButtonClicked;
        };

        DialogController(ConstructionArgs&& args);
        void showModal();
        void show();
        void hide();
        bool isOpen() const;
        void setClassName(std::string const& className);
        void setButtonClassName(std::string const& className);
        void setTitle(std::string const& title);
        void setBody(std::string const& body);
        void setButtonConfiguration(ButtonConfiguration buttons);
        void setOnButtonClicked(std::function<void(Button)> const& onButtonClicked);

        friend Nui::ElementRenderer Dialog(DialogController& controller);

      private:
        bool isOpen_;
        std::weak_ptr<Dom::Element> element_;
        ConstructionArgs args_;
    };
}