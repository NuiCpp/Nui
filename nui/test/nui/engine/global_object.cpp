#include "global_object.hpp"

namespace Nui::Tests::Engine
{
    Object globalObject;
    Object moduleObject;
    ReferenceType valueCounter;
    std::vector<Value> allValues;

    void resetGlobals()
    {
        globalObject = Object{};
        moduleObject = Object{};
        valueCounter = ReferenceType{0};
        allValues.clear();
    }
}