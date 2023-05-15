#include "global_object.hpp"

#include "function.hpp"
#include "value.hpp"
#include "object.hpp"
#include "array.hpp"

namespace Nui::Tests::Engine
{
    void installDocument()
    {
        globalObject["document"] = Object{};

        globalObject["document"]["location"] = Object{};
        globalObject["document"]["location"]["href"] = "file://dummy.html";
        globalObject["document"]["location"]["protocol"] = "file:";
        globalObject["document"]["location"]["host"] = "";
        globalObject["document"]["location"]["hostname"] = "";
        globalObject["document"]["location"]["port"] = "";
        globalObject["document"]["location"]["origin"] = "file://";
        globalObject["document"]["location"]["pathname"] = "dummy.html";

        globalObject["document"]["URL"] = "file://dummy.html";
        globalObject["document"]["body"] = Object{};

        globalObject["document"]["createElement"] = Function{[](std::string_view tag) -> Value {
            return Object{};
        }};
    }
}