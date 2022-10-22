project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        131eb285882fdad0892c898aee7e2cf1eb4aa2e7
)

FetchContent_MakeAvailable(roar)