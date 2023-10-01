#pragma once

#include <nui/frontend/event_system/event_context.hpp>

namespace Nui::Detail
{
    inline void makeChildrenUpdateEvent(auto& observedValues, auto& childrenRefabricator, auto& createdSelfWeak)
    {
        const auto eventId = Nui::globalEventContext.registerEvent(Event{
            [childrenRefabricator](int) -> bool {
                (*childrenRefabricator)();
                return false;
            },
            [createdSelfWeak]() {
                return !createdSelfWeak.expired();
            }});
        observedValues.attachOneshotEvent(eventId);
    }
}