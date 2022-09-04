#include "attribute.hpp"

#include <emscripten/val.h>

// click related
MAKE_HTML_EVENT_ATTRIBUTE(onClick)
MAKE_HTML_EVENT_ATTRIBUTE(onDblClick)
MAKE_HTML_EVENT_ATTRIBUTE(onMouseUp)
MAKE_HTML_EVENT_ATTRIBUTE(onMouseDown)

// cursor related
MAKE_HTML_EVENT_ATTRIBUTE(onMouseMove)
MAKE_HTML_EVENT_ATTRIBUTE(onMouseOut)
MAKE_HTML_EVENT_ATTRIBUTE(onMouseOver)
MAKE_HTML_EVENT_ATTRIBUTE(onMouseLeave)
MAKE_HTML_EVENT_ATTRIBUTE(onMouseEnter)

// other
MAKE_HTML_EVENT_ATTRIBUTE(onWheel)