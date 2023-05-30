#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/dom/element.hpp>

#include <functional>
#include <memory>

namespace Nui
{
    using ElementRenderer = std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>;
}