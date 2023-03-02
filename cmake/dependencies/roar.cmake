project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        66d87feec48000b50bf067a653cba5020636133f
)

FetchContent_MakeAvailable(roar)