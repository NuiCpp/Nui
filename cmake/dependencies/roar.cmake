project(roar-git NONE)

include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        63dc0187306603bd107c0afc58a17a4607d83769
)

FetchContent_MakeAvailable(roar)