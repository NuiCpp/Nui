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
  - Ubuntu (using clang20 and libc++) ![example workflow](https://github.com/NuiCpp/Nui/actions/workflows/ubuntu.yml/badge.svg)
  - Windows (using clang20 and current msys libstdc++) ![example workflow](https://github.com/NuiCpp/Nui/actions/workflows/windows.yml/badge.svg)
  - MacOS 13: (using clang16) ![example workflow](https://github.com/NuiCpp/Nui/actions/workflows/macos_13.yml/badge.svg)

Not automatically tested but should work:
  - Windows with Visual Studio, vcpkg and clang_cl
  - Arch Linux
  - MacOS 14+
  - Other Linux Distributions with new enough packages for C++20

Planed:
  - Android if this gets more traction here: https://github.com/webview/webview/issues/871

## Examples
Using SAPUI5 Components
![Example SAPUI5](https://user-images.githubusercontent.com/6238896/280534443-090023ca-8831-4423-83bf-e9d16c4a9b53.png)

To use them, see this repo: https://github.com/NuiCpp/ui5

Using Bootstrap and some custom CSS
![Example Bootstrap](https://nuicpp.org/assets/example.png)

Plenty found on https://nuicpp.org

## Contributing
Pull Requests are welcome, please format your changes and additions with clang-format (Ideally version 16+).

## Dependencies pulled by CMake
Nui uses the following dependencies:
- emscripten - [Custom LICENSE](https://github.com/emscripten-core/emscripten/blob/main/LICENSE)
- A nui maintained fork of webview/webview https://github.com/webview/webview - [MIT LICENSE](https://github.com/webview/webview/blob/master/LICENSE)
- boost under BSL License
- fmt https://github.com/fmtlib/fmt - [Custom MIT-like LICENSE](https://github.com/fmtlib/fmt/blob/master/LICENSE)
- gtest https://github.com/google/googletest - [BSD 3-Clause LICENSE](https://github.com/google/googletest/blob/main/LICENSE)
- 5cript/interval-tree https://github.com/5cript/interval-tree - [CC0-1.0 LICENSE](https://github.com/5cript/interval-tree/blob/master/LICENSE)
- 5cript/mplex https://github.com/5cript/mplex - [MIT LICENSE](https://github.com/5cript/mplex/blob/master/LICENSE)
- nlohmann/json https://github.com/nlohmann/json - [MIT LICENSE](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)
- portable-file-dialogs https://github.com/samhocevar/portable-file-dialogs - [WTFPL LICENSE](https://github.com/samhocevar/portable-file-dialogs/blob/main/COPYING)
- 5cript/roar https://github.com/5cript/roar - [BSL-1.0 LICENSE](https://github.com/5cript/roar/blob/master/LICENSE)
- Nui/traits https://github.com/NuiCpp/traits - [CC0-1.0 LICENSE](https://github.com/NuiCpp/traits/blob/main/LICENSE)
- nemtrif/utfcpp https://github.com/nemtrif/utfcpp - [BSL-1.0 license](https://github.com/nemtrif/utfcpp/blob/master/LICENSE)
