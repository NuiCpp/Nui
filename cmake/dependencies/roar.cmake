project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        ba4122904af057a33b37aa24d6b803d1f90f6d8d
)

FetchContent_MakeAvailable(roar)