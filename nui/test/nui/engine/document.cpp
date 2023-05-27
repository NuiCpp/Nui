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
        void cleanUndefinedDom(Nui::val v)
        {
            if (!v.isNull() && !v.isUndefined() && v.hasOwnProperty("children"))
            {
                auto& children = v["children"].template as<Array&>();
                children.clearUndefinedAndNull();
                for (auto& child : children)
                    cleanUndefinedDom(Nui::val{child});
            }
        }

        Nui::val createElement(Nui::val tag)
        {
            auto elem = Nui::val::object();
            elem.set("tagName", tag.template as<std::string>());
            elem.set("children", Nui::val::array());
            elem.set("appendChild", Function{[self = elem](Nui::val value) -> Nui::val {
                         return self["children"].template as<Array&>().push_back(value.handle());
                     }});
            elem.set("replaceWith", Function{[self = elem](Nui::val value) mutable -> Nui::val {
                         *self.handle() = *value.handle();
                         return self;
                     }});
            elem.set("remove", Function{[self = elem]() -> Nui::val {
                         allValues[*self.handle()] = nullptr;
                         if (!globalObject.has("document"))
                             return Nui::val::undefined();
                         if (Nui::val::global("document").hasOwnProperty("body"))
                             cleanUndefinedDom(Nui::val::global("document")["body"]);
                         return Nui::val::undefined();
                     }});
            elem.set("setAttribute", Function{[self = elem](Nui::val name, Nui::val value) -> Nui::val {
                         if (!self.template as<Object&>().has("attributes"))
                             self.set("attributes", createValue(Object{}));

                         self["attributes"].set(name.template as<std::string>(), *value.handle());

                         return Nui::val::undefined();
                     }});
            return elem;
        }
    }

    Document::Document()
    {
        globalObject.emplace("document", Object{});
        Nui::val::global("document").set("createElement", createElement);
        Nui::val::global("document").set("body", createElement("body"));
    }

    Nui::val Document::document()
    {
        return Nui::val::global("document");
    }
}