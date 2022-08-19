#include "attribute.hpp"

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