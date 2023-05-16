#include "document.hpp"

#include "function.hpp"
#include "value.hpp"
#include "object.hpp"
#include "array.hpp"
#include "global_object.hpp"

#include <iostream>

namespace Nui::Tests::Engine
{
    namespace
    {
        // inefficient but simple
        void cleanUndefinedDom(emscripten::val v)
        {
            if (v.hasOwnProperty("children"))
            {
                auto& children = v["children"].handle()->template as<Array&>();
                children.clearUndefined();
                for (auto& child : children)
                    cleanUndefinedDom(emscripten::val{child});
            }
        }

        emscripten::val createElement(emscripten::val tag)
        {
            auto elem = emscripten::val::object();
            elem.set("tagName", tag.template as<std::string>());
            elem.set("children", emscripten::val::array());
            elem.set("appendChild", Function{[self = elem.handle()](emscripten::val value) -> emscripten::val {
                         return (*self)["children"].template as<Array&>().push_back(value.handle());
                     }});
            elem.set("replaceWith", Function{[self = elem.handle()](emscripten::val value) -> emscripten::val {
                         *self = *value.handle();
                         return self;
                     }});
            elem.set("remove", Function{[self = elem.handle()]() -> emscripten::val {
                         *self = Value{};
                         cleanUndefinedDom(emscripten::val::global("document")["body"]);
                         return emscripten::val::undefined();
                     }});
            return elem;
        }
    }

    Document::Document()
    {
        globalObject.emplace_back("document", Object{});
        emscripten::val::global("document").set("createElement", createElement);
        emscripten::val::global("document").set("body", createElement("body"));
    }

    emscripten::val Document::document()
    {
        return emscripten::val::global("document");
    }
}