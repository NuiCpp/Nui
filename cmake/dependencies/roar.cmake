project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
    roar
    GIT_REPOSITORY https://github.com/5cript/roar.git
    GIT_TAG        c3ff20ff7e504745e70fd9715b955cfc0a05298e    
)

FetchContent_MakeAvailable(roar)