project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        304e215eb37bdf7bfdc209974eb2b7a3da1b8ae9
)

FetchContent_MakeAvailable(roar)