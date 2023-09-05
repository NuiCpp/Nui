target_sources(basic-example PRIVATE main.cpp main_page.cpp)

target_include_directories(basic-example PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../include)

# Link frontend of nui inside of emscripten
target_link_libraries(basic-example PRIVATE nui-frontend)

nui_prepare_emscripten_target(
    TARGET
        basic-example
    PARCEL_ARGS
    PREJS
        ${PREJS_FILE}
    EMSCRIPTEN_LINK_OPTIONS
        -sEXPORTED_FUNCTIONS=_frontendMain
        -O2
        -sALLOW_MEMORY_GROWTH=1 
    EMSCRIPTEN_COMPILE_OPTIONS
        -O3
)