project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        26f0c8d15e829979e258e0805717731611a69ad3
)

FetchContent_MakeAvailable(roar)