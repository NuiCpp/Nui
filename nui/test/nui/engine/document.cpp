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
            if (!v.isNull() && !v.isUndefined() && v.hasOwnProperty("children"))
            {
                auto& children = v["children"].template as<Array&>();
                children.clearUndefinedAndNull();
                for (auto& child : children)
                    cleanUndefinedDom(emscripten::val{child});
            }
        }

        emscripten::val createElement(emscripten::val tag)
        {
            auto elem = emscripten::val::object();
            elem.set("tagName", tag.template as<std::string>());
            elem.set("children", emscripten::val::array());
            elem.set("appendChild", Function{[self = elem](emscripten::val value) -> emscripten::val {
                         return self["children"].template as<Array&>().push_back(value.handle());
                     }});
            elem.set("replaceWith", Function{[self = elem](emscripten::val value) mutable -> emscripten::val {
                         *self.handle() = *value.handle();
                         return self;
                     }});
            elem.set("remove", Function{[self = elem]() -> emscripten::val {
                         allValues[*self.handle()] = nullptr;
                         if (!globalObject.has("document"))
                             return emscripten::val::undefined();
                         if (emscripten::val::global("document").hasOwnProperty("body"))
                             cleanUndefinedDom(emscripten::val::global("document")["body"]);
                         return emscripten::val::undefined();
                     }});
            elem.set(
                "setAttribute", Function{[self = elem](emscripten::val name, emscripten::val value) -> emscripten::val {
                    if (!self.template as<Object&>().has("attributes"))
                        self.set("attributes", createValue(Object{}));

                    self["attributes"].set(name.template as<std::string>(), *value.handle());

                    return emscripten::val::undefined();
                }});
            return elem;
        }
    }

    Document::Document()
    {
        globalObject.emplace("document", Object{});
        emscripten::val::global("document").set("createElement", createElement);
        emscripten::val::global("document").set("body", createElement("body"));
    }

    emscripten::val Document::document()
    {
        return emscripten::val::global("document");
    }
}