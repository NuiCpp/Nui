#include "attribute.hpp"

#include <emscripten/val.h>

// namespace Nui
// {
//     struct onClick_
//     {
//         ComplexAttribute operator=(std::function<void(emscripten::val)>&& func)
//         {
//             return {[f = std::move(func)](emscripten::val element) {
//                 element.set("onclick", js::bind(f, std::placeholders::_1));
//             }};
//         }
//     } onClick;
// }

namespace Nui::Attributes
{
    struct onClick_
    {
        constexpr static char const* name = "onclick";
        Attribute<onClick_, std::function<void(emscripten::val)>> operator=(std::function<void(emscripten::val)>&& func)
        {
            return Attribute<onClick_, std::function<void(emscripten::val)>>{std::move(func)};
        }
    } onClick;
}