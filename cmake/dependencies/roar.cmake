project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        49155256244a6d4fd24f3110222898d7f423570b
)

FetchContent_MakeAvailable(roar)