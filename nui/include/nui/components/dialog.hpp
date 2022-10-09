#pragma once

#include <nui/generator_typedefs.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/dom/element.hpp>

#include <functional>

namespace Nui::Components
{
    /**
     * @brief Examplary dialog class for the most simple kinds of dialogs.
     */
    class DialogController
    {
      public:
        enum class ButtonConfiguration
        {
            Ok,
            OkCancel,
            YesNo
        };

        struct ConstructionArgs
        {
            Observed<std::optional<std::string>> className_;
            Observed<std::string> titel_;
            Observed<std::string> body_;
            Observed<ButtonConfiguration> buttonConfiguration_;
        };

        DialogController(ConstructionArgs&& args);
        void showModal();
        void show();

        friend Nui::ElementRenderer Dialog(DialogController& controller);

      private:
        bool isOpen_;
        std::weak_ptr<Dom::Element> element_;
        ConstructionArgs args_;
    };
}