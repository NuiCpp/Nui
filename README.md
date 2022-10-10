# Nui
A C++ WebView UI Library

## Setup
### Linux
- Install `gtk+-3.0` and `webkit2gtk-4.0` for webview, more information here: https://github.com/webview/webview
### Windows - MSYS2
All is handled by cmake. You may use a system installed emscripten
### Windows - Visual Studio
Visual Studio and vc++ is currently not supported.

## Use system emscripten installation
Install emscripten globally and configure like so `cmake -DNUI_USE_EXTERNAL_EMSCRIPTEN=on`