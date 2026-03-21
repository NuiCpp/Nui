#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/dom/element.hpp>

#include <functional>
#include <memory>

namespace Nui
{
    /**
     * @brief The ElementRenderer is a foundational type in Nui's rendering system. The specifics of the function are
     * not interesting and only used internally, but the type is frequently used by the user of the library to represent
     * a factory for DOM elements.
     *
     * The function takes a reference to a parent DOM element and a 'Renderer' which is a misnomer and is more a
     * 'RenderStrategy', it determines how to create the elements within the parent. The function returns a shared
     * pointer to the created DOM element, which can then be inserted into the DOM kept by Nui or kept for delocalized
     * slots.
     */
    using ElementRenderer = std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>;
}