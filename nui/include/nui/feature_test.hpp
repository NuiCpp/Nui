#pragma once

#ifdef __has_include
#    if __has_include(<version>)
#        include <version>
#        ifdef __cpp_lib_ranges
#            ifndef NUI_HAS_STD_RANGES
#                define NUI_HAS_STD_RANGES 1
#            elif NUI_HAS_STD_RANGES == 0
#                undef NUI_HAS_STD_RANGES
#            endif
#        endif
#    endif
#endif