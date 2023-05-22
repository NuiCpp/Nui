#pragma once

#include <string_view>

namespace Nui::Tests::Engine
{
    /**
     * @brief Warns about something.
     *
     * @param message Message to warn about.
     */
    void warn(std::string_view message);
}