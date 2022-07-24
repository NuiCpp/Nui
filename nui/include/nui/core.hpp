#pragma once

#ifndef __EMSCRIPTEN__
#    ifdef EMSCRIPTEN_KEEPALIVE
#        undef EMSCRIPTEN_KEEPALIVE
#    endif
#    define EMSCRIPTEN_KEEPALIVE
#endif