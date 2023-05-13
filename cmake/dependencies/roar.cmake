project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        7b3528b21a2cbe06cf5d5d3a39fd559142cbeb94
)

FetchContent_MakeAvailable(roar)