#pragma once

#include <nui/frontend/event_system/observed_value.hpp>

namespace Nui
{
    /**
     * @brief Listens to fragment changes and changes the attached observed.
     */
    void listenToFragmentChanges(Observed<std::string>& fragment);
}