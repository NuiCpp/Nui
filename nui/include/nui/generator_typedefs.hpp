#pragma once

#include <nui/elements/html_element.hpp>
#include <nui/dom/element.hpp>

#include <functional>
#include <memory>

namespace Nui
{
    using ElementGenerator = std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Generator const&)>;
}