#include "../common/app.hpp"
#include "../common/render_process.hpp"

#include <include/base/cef_compiler_specific.h>
#include <include/cef_api_hash.h>
#include <include/cef_command_line.h>
#ifdef __APPLE__
#    include <include/cef_sandbox_mac.h>
#    include <include/wrapper/cef_library_loader.h>
#endif

#include <cstdio>

// Child processes of the CEF engine (zygote, renderer, GPU and utility processes). Chromium may change the stack guard
// after forking from this frame, so main must not use a stack protector, like CEF's own CefExecuteProcess.
NO_STACK_PROTECTOR
int main(int argc, char** argv)
{
#ifdef __APPLE__
    // The renderer checks that the seatbelt sandbox is active before Chromium code runs.
    CefScopedSandboxContext sandboxContext;
    if (!sandboxContext.Initialize(argc, argv))
        return 1;
    CefScopedLibraryLoader libraryLoader;
    if (!libraryLoader.LoadInHelper())
        return 1;
#endif

    if (cef_api_hash(CEF_API_VERSION, 0) == nullptr)
    {
        std::fprintf(stderr, "nui-cef-helper: the loaded libcef does not support CEF API version %d\n", CEF_API_VERSION);
        return 1;
    }

    const CefMainArgs mainArgs{argc, argv};
    auto commandLine = CefCommandLine::CreateCommandLine();
#ifdef _WIN32
    commandLine->InitFromString(::GetCommandLineW());
#else
    commandLine->InitFromArgv(argc, argv);
#endif

    CefRefPtr<CefApp> app = new Nui::Detail::Cef::App(
        Nui::Detail::Cef::customSchemesFromCommandLine(commandLine), nullptr, new Nui::Detail::Cef::RenderProcess(), {});
    return CefExecuteProcess(mainArgs, app, nullptr);
}
