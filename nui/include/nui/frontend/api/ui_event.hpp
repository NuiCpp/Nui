#pragma once

#include <nui/frontend/api/event.hpp>

namespace Nui::WebApi
{
    /**
     * @brief The UIEvent interface represents simple user interface events. It is part of the UI Events API, which
     * includes various event types and interfaces related to user interactions.
     *
     * @see https://developer.mozilla.org/en-US/docs/Web/API/UIEvent
     */
    class UiEvent : public Event
    {
      public:
        explicit UiEvent(Nui::val event);

        long detail() const;

        Nui::val sourceCapabilities() const;

        Nui::val view() const;
    };
}