# The CEF browser engine

Nui renders its frontend in the platform webview by default: WebKitGTK on Linux, WKWebView on macOS and WebView2 on
Windows. On Linux and macOS it can use the [Chromium Embedded Framework](https://github.com/chromiumembedded/cef)
(CEF) instead. The application code stays the same; the engine is a build option.

```bash
cmake -B build -DNUI_BROWSER_ENGINE=cef ...
```

`NUI_BROWSER_ENGINE` is `native` (default) or `cef`. CEF is not available on Windows, where WebView2 is already a
Chromium.

## What changes for an application

Nothing in a typical `main`:

```cpp
int main()
{
    Nui::Window window{Nui::WindowOptions{.title = "Hello", .debug = true}};
    window.setSize(480, 320, Nui::WebViewHint::WEBVIEW_HINT_NONE);
    window.setHtml(index());
    window.run();
}
```

- **Linux:** no CMake changes. Every executable that links `nui-backend` runs from the build tree.
- **macOS:** CEF needs an app bundle, also in the build tree. Call `nui_configure_application` once for the executable
  (it does nothing for the native engine, so it can stay in an engine neutral CMakeLists.txt):

  ```cmake
  nui_configure_application(TARGET my-app NAME "My App" IDENTIFIER com.example.myapp RESOURCES "${CMAKE_SOURCE_DIR}/assets")
  ```

  Call it in the directory that defines the target. Without it the app stops at startup with an error that names the
  function.
- **Installing:** `nui_install(TARGETS my-app)` works for both engines, see [Deployment](#deployment).

### Engine options

The engine starts at the first `Window::run` (or `Nui::Engine::start`). Settings that apply to the whole process go into
an optional `Nui::Engine`, created before that. It exists for both engines and does nothing with the native one.

```cpp
const Nui::Engine engine{Nui::EngineOptions{
    .browserArguments = {"--enable-features=Name"},
    .remoteDebuggingPort = 9222,
}};
```

| Option | Meaning |
|---|---|
| `customSchemes` | Scheme names of windows created after the engine started. Schemes of windows constructed before are registered automatically. |
| `browserArguments` | Extra Chromium switches. `--enable-features` and `--disable-features` are merged with Nui's defaults. `WindowOptions::browserArguments` of the first windows is added too. |
| `remoteDebuggingPort` | Chrome DevTools Protocol port, 0 picks a free one (`Engine::remoteDebuggingPort()` reads it). |
| `applicationId` | Name of the cache directory, default the executable name. |
| `desktopApplicationId` | Wayland app id and X11 `WM_CLASS`, default `g_get_prgname()`, `$FLATPAK_ID` or the executable name. Should match the `.desktop` file. |
| `cacheRoot` | Base of the cache directories, default `$XDG_CACHE_HOME` or `~/Library/Caches`; also `NUI_CEF_CACHE_ROOT`. |
| `helperPath` | The helper executable, found automatically otherwise. |
| `passProcessArguments` | Pass the process arguments to Chromium, so `--ozone-platform=x11` etc. work on the command line (default on). |
| `sandbox` | Linux: `Auto` (sandbox when the system supports it, otherwise a warning), `Required`, `Disabled`. |

The environment variable `NUI_CEF_ARGUMENTS` adds switches as well (for tests), `NUI_CEF_REMOTE_DEBUGGING_PORT` sets the
DevTools port when a window has `debug` enabled.

### Behaviour that differs from the native engines

- **One message loop per process.** The first `run()` runs until the last window has closed. Windows created while it
  runs (for example from an RPC handler) open right away and need no `run()`. After the loop has ended the engine cannot
  start again: creating a window then throws. Open the next window before the last one closes. A nested `run()` returns
  at once, so modal `run()` loops do not work. `Nui::Engine::KeepAlive` keeps the loop running without windows, for
  example while only native (non browser) windows are open.
- **`terminate()`** closes its window. The loop ends when it was the last one.
- **`setHtml`** serves the page at `https://app.example/index.html` (the `windowsServeThroughAuthority` argument), the
  same origin as WebView2. The native WebKit engines show it as `about:blank`. Authorities are lowercased like every URL
  host. A window serves only its current page: going back in history to the page of an earlier `setHtml` call shows
  404.
- **Custom scheme handlers** run on the UI thread, one request at a time, like on the native engines.
  `CustomScheme::handlerThread = SchemeHandlerThread::Any` runs them on worker threads in parallel. Range requests are
  answered automatically (206 with `Content-Range`, 416, `Accept-Ranges`) for `bodyFile`, `body` and `bodyReader`
  with a `Content-Length`; HEAD requests get the headers only. `bodyReader` calls run on a shared pool of 4 to 16
  threads and should not block for long: after the last window closed Nui waits at most 5 s for them, then leaves them
  behind. Request bodies of type Blob or stream do not reach custom schemes: reading
  them in the handler (`getContent`, `readContent`) throws, which answers 500.
- **Custom scheme names** must be known when the engine starts. Windows constructed before `run()` register theirs; a
  window created later with a new name throws, add the name to `EngineOptions::customSchemes`.
- **`bind` and `init` after the browser exists** are kept for every later document of the window, also in a new renderer
  process (cross site navigation, renderer crash), the native engines only evaluate them once. In the first document of
  a new renderer process they arrive a few milliseconds late, after the document's own inline scripts. Calls made
  before the browser exists are there from the start.
- **`getNativeWindow` and `getNativeWebView`** do not exist (they return GTK and WebKit objects on the native engines,
  so their callers would not work anyway). Use `Nui::Cef::nativeWindowHandle`, `Nui::Cef::browser` and
  `Nui::Cef::portalParentWindow` from `<nui/cef/client_hooks.hpp>`, which needs the `Nui::cef` CMake target.
- **Popups** (`target=_blank`, `window.open`) are blocked, like on the native engines without a handler.
- **Context menu, drag and drop, keyboard, permissions, navigation:** CEF handler objects can be passed with
  `Nui::Cef::setClientHooks(window, {...})` before the browser is created. Without a permission handler the app's own
  pages get clipboard access and everything else is denied. With a request handler, Range requests to the app's own
  origins reach Nui's scheme handlers with the header renamed to `X-Nui-Range` (`Nui::Cef::rangeHeaderName`) after the
  handler's `OnBeforeResourceLoad`, so Nui still answers them. A resource handler of the app for such a request finds
  the range under that name and answers it itself.
- **`runInJavascriptThread`** runs inline on the UI thread and is posted otherwise (WebKitGTK ran it inline on every
  thread).
- **`configureWebKitGtkRendering`** does nothing.
- **Signals:** the dispositions of SIGCHLD, SIGINT, SIGTERM and SIGHUP from before the engine starts are restored
  after it started (Chromium installs its own), so they behave as with the native engines. SIGCHLD must not be ignored
  and must not have SA_NOCLDWAIT, Chromium waits for its child processes.
- The frontend can check `globalThis.nui_rpc.engine` (`"cef"` or `"native"`).
- **`TMPDIR`** longer than 62 characters is replaced by `/tmp` for the process: Chromium's singleton socket below it must
  fit into a socket address, otherwise Chromium aborts.
- **Desktop portal identity (Linux, outside Flatpak):** Chromium moves the process into the systemd scope
  `app-org.chromium.Chromium-<pid>.scope`, a name Nui cannot change. xdg-desktop-portal then attributes portal requests
  of the app (file chooser, notifications) to `org.chromium.Chromium`. Inside Flatpak the portal uses the Flatpak id.
  An app that calls portals itself can register its own id on its D-Bus connection with
  `org.freedesktop.host.portal.Registry.Register` before its first portal call.

### Several windows and several instances

Windows of one process share the engine: one browser process, one GPU process. Each window has its own renderer
process, RPC functions and scheme handlers.

Chromium allows one process per cache directory and hands later launches to the first one. Nui gives every process its
own cache slot instead (`<cache>/<applicationId>/cef/<version>/<host>/<n>`, locked with `flock`), so two launches of
one binary, a debug next to a release build, or parallel tests are independent. A slot whose Chromium instance still
runs stays in use, also when a cache cleaner deleted Nui's lock file and in another pid namespace (Flatpak). Slots 0 to 3 are kept for their GPU
caches, higher ones and the directories of older CEF versions are removed when nobody uses them. The profile itself is
in memory (incognito): pages keep no storage.

## Getting CEF

One of these, checked in this order:

| Option | Use |
|---|---|
| `-DNUI_CEF_SYSTEM=ON` | The distribution's CEF package (Arch: `cef`, Fedora: `cef-devel`). Hints: `NUI_CEF_SYSTEM_LIBRARY_DIR`, `NUI_CEF_SYSTEM_INCLUDE_DIR`, `NUI_CEF_SYSTEM_WRAPPER_DIR`. |
| `-DNUI_CEF_ROOT=<dir>` | An extracted CEF binary distribution. |
| `-DNUI_CEF_ARCHIVE=<file>` | A downloaded distribution archive, extracted into the build tree. The pinned archive is checked against its SHA256. |
| `NUI_FETCH_CEF` (default on) | Downloads the pinned minimal distribution into `NUI_CEF_DOWNLOAD_DIR` (default `~/.cache/nui/cef`), SHA256 checked, shared by all build trees. |

Offline builds (Flatpak, Yocto, distribution packages) set `NUI_FETCH_CEF=OFF` and provide the archive or the system
package. To fetch ahead of time:

```bash
cmake -DNUI_CEF_DOWNLOAD_DIR=downloads [-DNUI_CEF_PLATFORM=linuxarm64] -P <Nui>/cmake/dependencies/cef_fetch.cmake
```

The pinned version, its URLs and SHA256 sums are in `cmake/dependencies/cef_version.cmake`. `NUI_CEF_API_VERSION`
selects the CEF API version, by default the newest one of the CEF headers in use, so a build against a system package
works with exactly that package and newer CEF versions that keep the API version.

Nui builds CEF's `libcef_dll_wrapper` from the distribution's sources itself and does not include CEF's CMake files,
which change global settings.

## Deployment

```cmake
nui_install(TARGETS my-app [DESTINATION bin] [ENGINE_DESTINATION lib/my-app] [COMPONENT runtime])
```

- **native:** `install(TARGETS)`.
- **CEF on Linux:** the executable to `DESTINATION` (default `bin`), CEF and the helper `nui-cef-helper` to
  `ENGINE_DESTINATION` (default `lib/<target>`), with a relative RPATH. `libcef.so` is stripped while installing
  (`NUI_CEF_STRIP`, 1.45 GB to 272 MB). `NUI_CEF_LOCALES` limits the installed locales (en-US is always kept),
  `NUI_CEF_SWIFTSHADER=OFF` leaves out the software renderer. The setuid `chrome-sandbox` is only installed with
  `NUI_CEF_INSTALL_CHROME_SANDBOX`: on most systems Chromium's sandbox uses user namespaces instead.
- **CEF on Linux with `NUI_CEF_SYSTEM`:** only the executable and the helper; both use the system libcef. A package then
  only depends on the distribution's `cef` package.
- **CEF on macOS:** the app bundle (framework and helper apps included), signed again after the copy. `NUI_CODESIGN_IDENTITY`
  selects the identity (default ad hoc), `NUI_CODESIGN_HARDENED_RUNTIME` signs for notarization (with the `allow-jit`
  entitlement CEF needs).

**Flatpak:** Chromium's sandbox needs [zypak](https://github.com/refi64/zypak) (from `org.electronjs.Electron2.BaseApp`)
with `ZYPAK_CEF_LIBRARY_PATH` set to the installed `libcef.so`. `nui_install(TARGETS my-app FLATPAK_ZYPAK)` installs the
executable next to CEF and a small launcher with its name to `DESTINATION` that does both, so the manifest's `command`
is just the app name. Without zypak, or with zypak but without the library path, Nui runs CEF without sandbox and logs
a warning instead of letting Chromium abort.

### Process model

All Chromium child processes (zygote, renderers, GPU, utilities) run `nui-cef-helper`, which Nui builds next to the CEF
runtime. The application's `main` never runs in a child process, so code before the first window (option parsing,
forking worker processes) runs once.

## Privacy

Chromium contacts Google services on its own. Nui turns this off: component updates, background networking, network
time, AI mode checks, search preconnects, the optimization guide model manifest (an empty manifest override in the
cache slot), account lookups (`--gaia-url` points to `gaia.localhost`, which a `--host-resolver-rules` entry placed
before the application's own rules keeps from resolving; an `EXCLUDE` rule of the application that matches it still
wins), spell checking and network prediction, which
would also look up the `setHtml` authority in DNS. With a fresh cache the app makes no request to the internet.
`browserArguments` can turn single features back on.

## Memory

Nui disables the spare renderer and the back-forward cache (which keeps a renderer process per page in history) and
runs the network service in the browser process on Linux. Measured with CEF 154:
8 instead of 10 processes and about 20 MiB less. More can be saved per application with `browserArguments`:
`--process-per-site` (windows of the same app share one renderer process) and `--in-process-gpu` (no GPU process, no GPU
sandbox). A spare renderer turned back on starts before later `setHtml` authorities exist and does not give pages from
them the RPC bridge. With the back-forward cache turned back on, pages restored from it lose the RPC bridge.
