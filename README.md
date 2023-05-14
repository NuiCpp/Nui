# Nui
A C++ WebView UI Library.

Temporary example page: http://5cript.net/

## Setup
### Linux
- Install `gtk+-3.0` and `webkit2gtk-4.0` for webview, more information here: https://github.com/webview/webview
### Windows - MSYS2
All is handled by cmake. boost, curl, openssl and crypto++ are required dependencies.
### Windows - Visual Studio
Visual Studio is supported using cmake and clang_cl (more compiler support might come in the future).
No extra setup is necessary thanks to vcpkg.

## Use system emscripten installation
Install emscripten globally and configure like so `cmake -DNUI_USE_EXTERNAL_EMSCRIPTEN=on`
