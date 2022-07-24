# Nui
A C++ WebView UI Library

## Setup
### Linux

- Install `gtk+-3.0` and `webkit2gtk-4.0` for webview, more information here: https://github.com/webview/webview
- Setup Emscripten __(choose one)__:
  1. Create a build directory and build this targets inside once: `emscripten_setup` using either `make emscripten_setup` or `cmake --build . --target emscripten_setup`
  2. Install emscripten globally and configure like so `cmake -DNUI_USE_EXTERNAL_EMSCRIPTEN=on`

### Windows - MSYS2

- Build the `webview2_msys` target once using either `make webview2_msys` or `cmake --build . --target webview2_msys`
- Setup Emscripten __(choose one)__:
  1. Build this targets inside once: `emscripten_setup` using either `make emscripten_setup` or `cmake --build . --target emscripten_setup`. Now modify the .emscripten file in $YOUR_BUILD_DIR/_deps/emscripten-src/upstream/emscripten/.emscripten. And set/fix the following variables:
      - EMSCRIPTEN_ROOT=$YOUR_BUILD_DIR/_deps/emscripten-src/upstream/emscripten
      - LLVM_ROOT=The directory that your llvm tools are in.
      - BINARYEN_ROOT=This directory should contain a "bin/wasm-opt"
      - NODE_JS=$YOUR_BUILD_DIR/_deps/emscripten-src/node/VERSION/bin/node.exe
      - JAVA=$YOUR_BUILD_DIR/_deps/emscripten-src/java/VERSION/bin/java.exe
  2. Install emscripten globally and configure like so `cmake -DNUI_USE_EXTERNAL_EMSCRIPTEN=on`