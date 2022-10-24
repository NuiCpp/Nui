project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        4de47f7d3f4ab846fa151b326660abe4d60d1515
)

FetchContent_MakeAvailable(roar)