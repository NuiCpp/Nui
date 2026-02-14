cmake_minimum_required(VERSION 3.16)

project(xml-to-nui VERSION 0.1.0)

add_executable(xml-to-nui main.cpp)
target_compile_features(xml-to-nui PRIVATE cxx_std_20)

find_package(pugixml REQUIRED)
find_package(fmt REQUIRED)
find_package(Boost CONFIG 1.81.0 REQUIRED COMPONENTS program_options)

target_link_libraries(
    xml-to-nui
    PRIVATE
        pugixml::pugixml
        fmt::fmt
        Boost::program_options
)

file(GLOB NUI_ATTRIBUTE_HEADERS LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/attributes" "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/attributes/*.hpp")
# Cut off hpp and remove underscore
set(NUI_ATTRIBUTES "")
foreach(header IN LISTS NUI_ATTRIBUTE_HEADERS)
    string(REPLACE ".hpp" "" attributeName ${header})
    string(REPLACE "_" "" attributeName ${attributeName})
    list(APPEND NUI_ATTRIBUTES ${attributeName})
endforeach()

file(GLOB NUI_ELEMENT_HEADERS LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/elements" "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/elements/*.hpp")
# Cut off hpp and remove underscore
set(NUI_ELEMENTS "")
foreach(header IN LISTS NUI_ELEMENT_HEADERS)
    string(REPLACE ".hpp" "" elementName ${header})
    string(REPLACE "_" "" elementName ${elementName})
    list(APPEND NUI_ELEMENTS ${elementName})
endforeach()

file(GLOB NUI_SVG_ATTRIBUTE_HEADERS LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/attributes/svg" "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/attributes/svg/*.hpp")
# Cut off hpp and remove underscore
set(NUI_SVG_ATTRIBUTES "")
foreach(header IN LISTS NUI_SVG_ATTRIBUTE_HEADERS)
    string(REPLACE ".hpp" "" attributeName ${header})
    string(REPLACE "_" "" attributeName ${attributeName})
    list(APPEND NUI_SVG_ATTRIBUTES ${attributeName})
endforeach()

file(GLOB NUI_SVG_ELEMENT_HEADERS LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/elements/svg" "${CMAKE_CURRENT_LIST_DIR}/../../nui/include/nui/frontend/elements/svg/*.hpp")
# Cut off hpp and remove underscore
set(NUI_SVG_ELEMENTS "")
foreach(header IN LISTS NUI_SVG_ELEMENT_HEADERS)
    string(REPLACE ".hpp" "" elementName ${header})
    string(REPLACE "_" "" elementName ${elementName})
    list(APPEND NUI_SVG_ELEMENTS ${elementName})
endforeach()

string(REPLACE ";" "," NUI_ATTRIBUTES "${NUI_ATTRIBUTES}")
string(REPLACE ";" "," NUI_ELEMENTS "${NUI_ELEMENTS}")
string(REPLACE ";" "," NUI_SVG_ATTRIBUTES "${NUI_SVG_ATTRIBUTES}")
string(REPLACE ";" "," NUI_SVG_ELEMENTS "${NUI_SVG_ELEMENTS}")

target_compile_definitions(xml-to-nui PRIVATE
    NUI_ATTRIBUTES="${NUI_ATTRIBUTES}"
    NUI_ELEMENTS="${NUI_ELEMENTS}"
    NUI_SVG_ATTRIBUTES="${NUI_SVG_ATTRIBUTES}"
    NUI_SVG_ELEMENTS="${NUI_SVG_ELEMENTS}"
)

set_target_properties(xml-to-nui
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools_bin"
)