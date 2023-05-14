project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        9b01fa6d5dac507b9ed86022e3422089fb851e1b
)

FetchContent_MakeAvailable(roar)