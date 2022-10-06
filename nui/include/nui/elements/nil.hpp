#pragma once

#include <nui/elements/html_element.hpp>

namespace Nui
{
    auto nil()
    {
        return []<typename T>(auto& parentElement, GeneratorOptions<T> const&) {
            return std::shared_ptr<std::decay_t<decltype(parentElement)>>(nullptr);
        };
    }
}