#include <emscripten/val.h>

namespace Nui
{
    /// Nui val is currently just a typedef for emscripten val. Nui reserves the right to change this in the future.
    using val = emscripten::val;
}