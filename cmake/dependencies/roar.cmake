include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        3c9fdde93d0f70f0d0dd32c68f10587ef43dce9d
)

FetchContent_MakeAvailable(roar)