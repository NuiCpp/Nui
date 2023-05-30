#pragma once

#include <nui/frontend/element_renderer.hpp>
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
            Observed<std::optional<std::string>> className = std::nullopt;
            Observed<std::string> title = "";
            Observed<std::string> body = "";
            Observed<std::string> buttonClassName = "";
            Observed<ButtonConfiguration> buttonConfiguration = ButtonConfiguration::Ok;
            std::function<void(Button)> onButtonClicked = [](Button) {};
        };

        /**
         * @brief Constructs a new DialogController.
         *
         * @param args The intial values for the dialog.
         */
        DialogController(ConstructionArgs&& args);

        /**
         * @brief Shows the dialog as a modal dialog (blocks the UI).
         */
        void showModal();

        /**
         * @brief Shows the dialog as a non-modal dialog (does not block the UI).
         */
        void show();

        /**
         * @brief Hides the dialog.
         */
        void hide();

        /**
         * @brief Returns true if the dialog is open.
         */
        bool isOpen() const;

        /**
         * @brief Sets the class name of the dialog.
         */
        void setClassName(std::string const& className);

        /**
         * @brief Sets the class name of the dialog buttons.
         */
        void setButtonClassName(std::string const& className);

        /**
         * @brief Sets the title of the dialog.
         */
        void setTitle(std::string const& title);

        /**
         * @brief Sets the body of the dialog.
         */
        void setBody(std::string const& body);

        /**
         * @brief Sets the button configuration of the dialog.
         */
        void setButtonConfiguration(ButtonConfiguration buttons);

        /**
         * @brief Sets the callback that is called when a button is clicked.
         */
        void setOnButtonClicked(std::function<void(Button)> const& onButtonClicked);

        /**
         * @brief Creates a dialog element with the specified controller.
         *
         * @param controller An instance of DialogController.
         */
        friend Nui::ElementRenderer Dialog(DialogController& controller);

      private:
        bool isOpen_;
        std::weak_ptr<Dom::Element> element_;
        ConstructionArgs args_;
    };
}