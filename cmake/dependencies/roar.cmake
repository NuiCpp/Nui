project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        edaa851defaee0e33ed3ee3cb90232d2ee7ce64c
)

FetchContent_MakeAvailable(roar)