#pragma once

#include "object.hpp"

namespace Nui::Tests::Engine
{
    static Object globalObject;
    static Object moduleObject;

    /**
     * @brief Creates document object in global object.
     */
    void installDocument();
}