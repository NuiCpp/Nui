option(NUI_FETCH_UTFCPP "Try FetchContent for utfcpp" ON)

set(NUI_UTFCPP_GIT_REPOSITORY "https://github.com/nemtrif/utfcpp.git" CACHE STRING "utfcpp git repository")
set(NUI_UTFCPP_GIT_TAG "v4.0.8" CACHE STRING "utfcpp git tag")

include("${CMAKE_CURRENT_LIST_DIR}/../fetcher.cmake")

nui_fetch_dependency(
    LIBRARY_NAME utf8cpp
    FIND OFF # Developer does not support this library being found
    FETCH ${NUI_FETCH_UTFCPP}
    GIT_REPOSITORY ${NUI_UTFCPP_GIT_REPOSITORY}
    GIT_TAG ${NUI_UTFCPP_GIT_TAG}
)