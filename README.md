# Nui
A C++ WebView UI Library.
Nui enables you to build C++ user interfaces using a browser window tied to a main process.

Website: https://nuicpp.org

Documentation: https://nuicpp.github.io/nui-documentation/

## Setup
How to get going with nui is explained here in detail: [Installation](https://nuicpp.github.io/nui-documentation/docs/getting_started/installation)
A template to get going can be found here: [Template](https://github.com/NuiCpp/nui-template)

## Platforms & Compilers
Tested on:
  - Ubuntu 20 (using clang15 and libstdc++10) ![example workflow](https://github.com/NuiCpp/Nui/actions/workflows/ubuntu_20.yml/badge.svg)
  - Ubuntu 22 (using clang14 and libstdc++12) ![example workflow](https://github.com/NuiCpp/Nui/actions/workflows/ubuntu_22.yml/badge.svg)
  - Windows (using clang16 and current msys libstdc++) ![example workflow](https://github.com/NuiCpp/Nui/actions/workflows/windows.yml/badge.svg)

Untested but should work:
  - Arch Linux
  - Other Linux Distributions

Planed:
  - Mac OS
  - Android if this gets more traction here: https://github.com/webview/webview/issues/871

## Examples
![Example Ui](https://nuicpp.org/assets/example.png)

Plenty found on https://nuicpp.org

## Contributing
Pull Requests are welcome, please format your changes and additions with clang-format (Ideally version 16+).

## Dependencies pulled by CMake
Nui uses the following dependencies:
- emscripten - [LICENSE](https://github.com/emscripten-core/emscripten/blob/main/LICENSE)
- A nui maintained fork of webview/webview https://github.com/webview/webview - [LICENSE](https://github.com/webview/webview/blob/master/LICENSE)
- boost under BSL License
- fmt https://github.com/fmtlib/fmt - [LICENSE](https://github.com/fmtlib/fmt/blob/master/LICENSE)
- gtest https://github.com/google/googletest - [BSL 3.0 LICENSE](https://github.com/google/googletest/blob/main/LICENSE)
- 5cript/interval-tree https://github.com/5cript/interval-tree - [CC0 LICENSE](https://github.com/5cript/interval-tree/blob/master/LICENSE)
- 5cript/mplex https://github.com/5cript/mplex - [MIT LICENSE](https://github.com/5cript/mplex/blob/master/LICENSE)
- nlohmann/json https://github.com/nlohmann/json - [MIT LICENSE](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)
- portable-file-dialogs https://github.com/samhocevar/portable-file-dialogs - [WTFPL LICENSE](https://github.com/samhocevar/portable-file-dialogs/blob/main/README.md)
- 5cript/roar https://github.com/5cript/roar - [BSL-1.0 LICENSE](https://github.com/5cript/roar/blob/master/LICENSE)