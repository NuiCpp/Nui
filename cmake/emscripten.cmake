project(emscripten-git NONE)

include(FetchContent)
FetchContent_Declare(
    emscripten
    GIT_REPOSITORY https://github.com/emscripten-core/emsdk.git
    GIT_TAG        961e66c5099e1119dda25f7a8945d155fc561c90    
)

FetchContent_MakeAvailable(emscripten)

add_custom_target(
    emscripten_setup
    ALL
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/emscripten-src"
    #COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" install latest
    COMMAND "${CMAKE_BINARY_DIR}/_deps/emscripten-src/emsdk" activate latest
    COMMAND_EXPAND_LISTS
)