include(FetchContent)
FetchContent_Declare(
	roar
	GIT_REPOSITORY https://github.com/5cript/roar.git
	GIT_TAG        560f5cd4ea59fb4f9e667aed485b52794680adcf
)

FetchContent_MakeAvailable(roar)