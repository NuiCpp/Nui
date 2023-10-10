option(NUI_USE_EXTERNAL_EMSCRIPTEN "Use external emscripten?" off)

option(NUI_ENABLE_TESTS "Enable test target" off)
set(NUI_NPM "npm" CACHE STRING "Path to npm (node package manager)")
set(NUI_NODE "node" CACHE STRING "Path to node")
option(NUI_BUILD_EXAMPLES "Build examples" off)
option(NUI_ENABLE_CLANG_TIDY "Enable clang-tidy" off)