project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        80a55a65ae2d3198a42b3a7f1da277aed9db964f
)

FetchContent_MakeAvailable(roar)