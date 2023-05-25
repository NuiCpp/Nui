project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        60324320cb80caec5428b0cff0c0045ed27668e8
)

FetchContent_MakeAvailable(roar)