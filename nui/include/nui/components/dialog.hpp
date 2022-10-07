#pragma once

#include <nui/generator_typedefs.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/dom/element.hpp>

#include <functional>

namespace Nui::Components
{
    class DialogController
    {
      public:
        DialogController();
        void showModal();
        void show();

        friend Nui::ElementRenderer Dialog(DialogController& controller);

      private:
        bool isOpen_;
        std::weak_ptr<Dom::Element> element_;
    };
}