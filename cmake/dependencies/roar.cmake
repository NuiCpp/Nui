project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        ca8849723ec723cabb5b2dd321a53ace76cc5525
)

FetchContent_MakeAvailable(roar)