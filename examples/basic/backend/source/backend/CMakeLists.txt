target_sources(basic-example PRIVATE main.cpp)

target_include_directories(basic-example PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../include)

# Link backend of nui outside of emscripten
target_link_libraries(basic-example PRIVATE nui-backend)

set_target_properties(basic-example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples/basic-example"
)

# Creates a target that is compiled through emscripten. This target becomes the frontend part.
nui_add_emscripten_target(
    TARGET
        basic-example
    PREJS
        ${PREJS_FILE}
    SOURCE_DIR
        ${CMAKE_SOURCE_DIR}
    CMAKE_OPTIONS
        "-DNUI_BUILD_EXAMPLES=on"
        # I recommend to work with a release build by default because debug builds get big fast.
        "-DCMAKE_BUILD_TYPE=Release"
)