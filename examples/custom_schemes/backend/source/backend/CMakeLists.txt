target_sources(${PROJECT_NAME} PRIVATE main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../include)

# Link backend of nui outside of emscripten
target_link_libraries(${PROJECT_NAME} PRIVATE nui-backend)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples/${PROJECT_NAME}"
)

# Creates a target that is compiled through emscripten. This target becomes the frontend part.
nui_add_emscripten_target(
    TARGET
        ${PROJECT_NAME}
    PREJS
        ${PREJS_FILE}
    SOURCE_DIR
        ${CMAKE_SOURCE_DIR}
    CMAKE_OPTIONS
        "-DNUI_BUILD_EXAMPLES=on"
        # I recommend to work with a release build by default because debug builds get big fast.
        "-DCMAKE_BUILD_TYPE=Release"
)