#pragma once

#include <nui/elements/impl/html_element.hpp>
#include <nui/dom/element.hpp>

#include <functional>
#include <memory>

namespace Nui
{
    using ElementRenderer = std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>;
}