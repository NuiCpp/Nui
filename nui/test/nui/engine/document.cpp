#include "document.hpp"

#include "function.hpp"
#include "value.hpp"
#include "object.hpp"
#include "array.hpp"
#include "global_object.hpp"

namespace Nui::Tests::Engine
{
    namespace
    {
        emscripten::val createElement(std::string tag)
        {
            auto elem = emscripten::val::object();
            elem["tagName"] = tag;
            elem["children"] = emscripten::val::array();
            elem["appendChild"] = Function{[weak = elem.handle()](emscripten::val const& value) -> emscripten::val {
                auto parent = weak.lock();
                if (!parent)
                    return emscripten::val{};

                return (*parent)["children"].template as<Array&>().push_back(value.handle().lock());
            }};
            return elem;
        }
    }

    Document::Document()
    {
        globalObject.emplace_back("document", Object{});
        emscripten::val::global("document").set("createElement", createElement);
    }

    emscripten::val Document::document()
    {
        return emscripten::val::global("document");
    }
}