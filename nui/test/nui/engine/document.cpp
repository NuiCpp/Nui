#include "document.hpp"

#include "function.hpp"
#include "value.hpp"
#include "object.hpp"
#include "array.hpp"
#include "global_object.hpp"

#include <iostream>
#include <algorithm>

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
                auto& nodes = v["childNodes"].template as<Array&>();
                nodes.clearUndefinedAndNull();
                children.clearUndefinedAndNull();
                for (auto& child : children)
                    cleanUndefinedDom(Nui::val{child});
            }
        }

        Nui::val createBasicElement(Nui::val tag)
        {
            auto elem = Nui::val::object();
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
            elem.set("nodeValue", Nui::val::null());
            elem.set("namespaceURI", Nui::val::null());
            elem.set("childNodes", Nui::val::array());
            elem.set("children", Nui::val::array());
            elem.set("tagName", tag.template as<std::string>());
            elem.set("nodeType", int{-1});
            return elem;
        }

        Nui::val createElement(Nui::val tag)
        {
            auto elem = createBasicElement(tag);
            elem.set("nodeType", int{1});
            elem.set("appendChild", Function{[self = elem](Nui::val value) -> Nui::val {
                         if (value.hasOwnProperty("parentNode"))
                             value["parentNode"].call<void>("removeChild", value);
                         value.set("parentNode", self);
                         self["childNodes"].template as<Array&>().push_back(value.handle());
                         return self["children"].template as<Array&>().push_back(value.handle());
                     }});
            elem.set("setAttribute", Function{[self = elem](Nui::val name, Nui::val value) -> Nui::val {
                         if (!self.template as<Object&>().has("attributes"))
                             self.set("attributes", createValue(Object{}));

                         self["attributes"].set(name.template as<std::string>(), *value.handle());

                         return Nui::val::undefined();
                     }});
            elem.set("removeAttribute", Function{[self = elem](Nui::val name) -> Nui::val {
                         if (!self.template as<Object&>().has("attributes"))
                             return Nui::val::undefined();

                         self["attributes"].delete_(name.template as<std::string>());

                         return Nui::val::undefined();
                     }});
            elem.set("removeChild", Function{[self = elem](Nui::val value) -> Nui::val {
                         auto eraseIt = [&](auto& container) {
                             auto it = std::find(container.begin(), container.end(), value.handle());
                             if (it != container.end())
                                 container.erase(it);
                         };
                         eraseIt(value["children"].template as<Array&>());
                         eraseIt(value["childNodes"].template as<Array&>());
                         return Nui::val::undefined();
                     }});
            return elem;
        }

        Nui::val createElementNs(Nui::val ns, Nui::val tag)
        {
            auto elem = createElement(tag);
            elem.set("namespaceURI", ns);
            return elem;
        }

        Nui::val createTextNode(Nui::val text)
        {
            auto elem = createBasicElement(Nui::val{std::string{}});
            elem.set("nodeType", int{3});
            elem.set("nodeValue", text);
            elem.set("appendChild", Function{[self = elem](Nui::val value) -> Nui::val {
                         value.set("parentNode", self);
                         return self["childNodes"].template as<Array&>().push_back(value.handle());
                     }});
            return elem;
        }
    }

    Document::Document()
    {
        globalObject.emplace("document", Object{});
        Nui::val::global("document").set("createElement", createElement);
        Nui::val::global("document").set("createElementNS", createElementNs);
        Nui::val::global("document").set("createTextNode", createTextNode);
        Nui::val::global("document").set("body", createElement("body"));
    }

    Nui::val Document::document()
    {
        return Nui::val::global("document");
    }
}