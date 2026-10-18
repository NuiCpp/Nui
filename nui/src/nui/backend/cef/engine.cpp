#include "engine_state.hpp"
#include "instance_slot.hpp"
#include "scheme_handler.hpp"
#include "task.hpp"
#include "common/app.hpp"
#include "common/messages.hpp"
#ifdef __APPLE__
#    include "mac/application_mac.hpp"
#else
#    include "sandbox_linux.hpp"
#endif

#include <nui/engine.hpp>

#include <include/cef_api_hash.h>
#include <include/cef_app.h>
#include <include/cef_command_line.h>
#include <include/cef_request_context.h>
#include <include/cef_scheme.h>
#include <include/cef_values.h>
#include <include/cef_version_info.h>

#include <boost/asio/thread_pool.hpp>
#include <fmt/format.h>

#ifdef __APPLE__
#    include <crt_externs.h>
#    include <mach-o/dyld.h>
#    include <pthread.h>
#else
#    include <dlfcn.h>
#    include <glib.h>
#    include <link.h>
#endif
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef __APPLE__
/// Path of the helper relative to the executable, recorded by nui_install for layouts where the helper is not next to
/// libcef (system CEF).
extern "C" [[gnu::weak]] char const nuiCefHelperRelativePath[];
#endif

namespace Nui::Detail::Cef
{
    namespace
    {
        std::filesystem::path executablePath()
        {
#ifdef __APPLE__
            std::uint32_t size = 0u;
            _NSGetExecutablePath(nullptr, &size);
            std::string path(size, '\0');
            _NSGetExecutablePath(path.data(), &size);
            std::error_code error;
            auto canonical = std::filesystem::canonical(path.c_str(), error);
            return error ? std::filesystem::path{path.c_str()} : canonical;
#else
            std::error_code error;
            return std::filesystem::canonical("/proc/self/exe", error);
#endif
        }

        std::vector<std::string> splitList(std::string_view text, char separator)
        {
            std::vector<std::string> result;
            while (!text.empty())
            {
                const auto end = text.find(separator);
                const auto item = text.substr(0, end);
                if (!item.empty())
                    result.emplace_back(item);
                if (end == std::string_view::npos)
                    break;
                text.remove_prefix(end + 1);
            }
            return result;
        }

        std::string joinList(std::vector<std::string> const& items, char separator)
        {
            std::string result;
            for (auto const& item : items)
            {
                if (!result.empty())
                    result += separator;
                result += item;
            }
            return result;
        }

        std::vector<std::string> tokenizeArguments(std::string_view text)
        {
            std::vector<std::string> tokens;
            std::string current;
            char quote = '\0';
            bool inToken = false;
            for (const auto character : text)
            {
                if (quote != '\0')
                {
                    if (character == quote)
                        quote = '\0';
                    else
                        current += character;
                }
                else if (character == '"' || character == '\'')
                {
                    quote = character;
                    inToken = true;
                }
                else if (character == ' ' || character == '\t' || character == '\n')
                {
                    if (inToken)
                        tokens.push_back(std::move(current));
                    current.clear();
                    inToken = false;
                }
                else
                {
                    current += character;
                    inToken = true;
                }
            }
            if (inToken)
                tokens.push_back(std::move(current));
            return tokens;
        }

        /// Arguments of the process for CefMainArgs, kept alive for the process lifetime.
        struct ProcessArguments
        {
            std::vector<std::string> storage{};
            std::vector<char*> pointers{};
        };

        ProcessArguments& processArguments(bool all)
        {
            static ProcessArguments arguments;
            arguments.storage.clear();
#ifdef __APPLE__
            const int count = *_NSGetArgc();
            char** values = *_NSGetArgv();
            for (int index = 0; index < count && (all || index == 0); ++index)
                arguments.storage.emplace_back(values[index]);
#else
            std::ifstream commandLine{"/proc/self/cmdline", std::ios::binary};
            std::string argument;
            while (std::getline(commandLine, argument, '\0'))
            {
                arguments.storage.push_back(argument);
                if (!all)
                    break;
            }
#endif
            if (arguments.storage.empty())
                arguments.storage.emplace_back(executablePath().string());
            arguments.pointers.clear();
            for (auto& value : arguments.storage)
                arguments.pointers.push_back(value.data());
            arguments.pointers.push_back(nullptr);
            return arguments;
        }

        std::filesystem::path userCacheDirectory()
        {
            if (auto const* root = std::getenv("NUI_CEF_CACHE_ROOT"); root != nullptr && *root != '\0')
                return root;
#ifdef __APPLE__
            if (auto const* home = std::getenv("HOME"); home != nullptr && *home != '\0')
                return std::filesystem::path{home} / "Library" / "Caches";
#else
            if (auto const* cacheHome = std::getenv("XDG_CACHE_HOME"); cacheHome != nullptr && *cacheHome != '\0')
                return cacheHome;
            if (auto const* home = std::getenv("HOME"); home != nullptr && *home != '\0')
                return std::filesystem::path{home} / ".cache";
#endif
            return std::filesystem::temp_directory_path();
        }

        std::string loadedCefVersion()
        {
            return fmt::format("{}.{}.{}", cef_version_info(0), cef_version_info(1), cef_version_info(2));
        }

#ifndef __APPLE__
        std::optional<std::filesystem::path> libcefDirectory()
        {
            void* handle = ::dlopen("libcef.so", RTLD_LAZY | RTLD_NOLOAD);
            if (handle == nullptr)
                return std::nullopt;
            link_map* map = nullptr;
            std::optional<std::filesystem::path> directory;
            if (::dlinfo(handle, RTLD_DI_LINKMAP, &map) == 0 && map != nullptr && map->l_name != nullptr)
                directory = std::filesystem::path{map->l_name}.parent_path();
            ::dlclose(handle);
            return directory;
        }

        std::filesystem::path findHelper(EngineOptions const& options)
        {
            if (options.helperPath)
                return *options.helperPath;
            std::vector<std::filesystem::path> candidates;
            if (const auto directory = libcefDirectory())
                candidates.push_back(*directory / "nui-cef-helper");
            const auto executableDirectory = executablePath().parent_path();
            if (&nuiCefHelperRelativePath[0] != nullptr)
                candidates.push_back((executableDirectory / nuiCefHelperRelativePath).lexically_normal());
            candidates.push_back(executableDirectory / "nui-cef-helper");
            for (auto const& candidate : candidates)
            {
                if (::access(candidate.c_str(), X_OK) == 0)
                    return candidate;
            }
            std::string searched;
            for (auto const& candidate : candidates)
                searched += "\n  " + candidate.string();
            throw std::runtime_error("Nui: the CEF helper executable was not found, searched:" + searched);
        }

        std::string desktopIdentity(EngineOptions const& options)
        {
            if (options.desktopApplicationId)
                return *options.desktopApplicationId;
            if (auto const* programName = g_get_prgname(); programName != nullptr && *programName != '\0')
                return programName;
            if (auto const* flatpakId = std::getenv("FLATPAK_ID"); flatpakId != nullptr && *flatpakId != '\0')
                return flatpakId;
            return executablePath().filename().string();
        }
#endif

        /**
         * @brief Signal handlers of the application, restored after CefInitialize.
         */
        class SignalSnapshot
        {
          public:
            SignalSnapshot()
            {
                for (const auto signalNumber : {SIGCHLD, SIGINT, SIGTERM, SIGHUP})
                {
                    struct sigaction action{};
                    ::sigaction(signalNumber, nullptr, &action);
                    actions_.emplace_back(signalNumber, action);
                }
                ::pthread_sigmask(SIG_SETMASK, nullptr, &mask_);
            }

            bool childSignalIgnored() const
            {
                return actions_.front().second.sa_handler == SIG_IGN;
            }

            void restore() const
            {
                for (auto const& [signalNumber, action] : actions_)
                {
                    if ((action.sa_flags & SA_SIGINFO) != 0 || action.sa_handler != SIG_DFL)
                        ::sigaction(signalNumber, &action, nullptr);
                }
                ::pthread_sigmask(SIG_SETMASK, &mask_, nullptr);
            }

          private:
            std::vector<std::pair<int, struct sigaction>> actions_{};
            sigset_t mask_{};
        };

        struct CommandLineSettings
        {
            std::vector<std::string> browserArguments{};
            bool disableSandbox{false};
            bool pickRemoteDebuggingPort{false};
        };

        void applyArgument(CefRefPtr<CefCommandLine> const& commandLine, std::string_view argument,
                           std::vector<std::string>& enableFeatures, std::vector<std::string>& disableFeatures)
        {
            while (argument.starts_with('-'))
                argument.remove_prefix(1);
            if (argument.empty())
                return;
            const auto equals = argument.find('=');
            const auto name = std::string{argument.substr(0, equals)};
            const auto value = equals == std::string_view::npos ? std::string{} : std::string{argument.substr(equals + 1)};
            if (name == "enable-features" || name == "disable-features")
            {
                auto& list = name == "enable-features" ? enableFeatures : disableFeatures;
                auto& opposite = name == "enable-features" ? disableFeatures : enableFeatures;
                for (auto const& feature : splitList(value, ','))
                {
                    std::erase(opposite, feature);
                    if (std::ranges::find(list, feature) == list.end())
                        list.push_back(feature);
                }
                return;
            }
            if (commandLine->HasSwitch(name))
                commandLine->RemoveSwitch(name);
            if (equals == std::string_view::npos)
                commandLine->AppendSwitch(name);
            else
                commandLine->AppendSwitchWithValue(name, value);
        }

        void adjustBrowserCommandLine(CefRefPtr<CefCommandLine> const& commandLine, CommandLineSettings const& settings)
        {
            // Chromium would load extensions installed for the system browser (KDE's Plasma Browser Integration).
            commandLine->AppendSwitch("disable-extensions");
            // Nui keeps no passwords or encrypted cookies, this keeps Chromium away from Keychain, KWallet and libsecret.
#ifdef __APPLE__
            commandLine->AppendSwitch("use-mock-keychain");
#else
            if (!commandLine->HasSwitch("password-store"))
                commandLine->AppendSwitchWithValue("password-store", "basic");
#endif
            // No component downloads and background requests to Google services.
            commandLine->AppendSwitch("disable-component-update");
            commandLine->AppendSwitch("disable-background-networking");

            // CEF puts its own feature defaults on the command line before this point, they are kept.
            auto enableFeatures = splitList(commandLine->GetSwitchValue("enable-features").ToString(), ',');
            auto disableFeatures = splitList(commandLine->GetSwitchValue("disable-features").ToString(), ',');
            // An idle spare renderer only speeds up the second window, it costs a process and memory.
            disableFeatures.emplace_back("SpareRendererForSitePerProcess");
#ifndef __APPLE__
            // The network service is not sandboxed on Linux, in process it saves a process and memory.
            enableFeatures.emplace_back("NetworkServiceInProcess2");
#endif
            for (auto const& argument : settings.browserArguments)
                applyArgument(commandLine, argument, enableFeatures, disableFeatures);
            commandLine->RemoveSwitch("enable-features");
            commandLine->RemoveSwitch("disable-features");
            if (!enableFeatures.empty())
                commandLine->AppendSwitchWithValue("enable-features", joinList(enableFeatures, ','));
            if (!disableFeatures.empty())
                commandLine->AppendSwitchWithValue("disable-features", joinList(disableFeatures, ','));

            if (settings.disableSandbox)
                commandLine->AppendSwitch("no-sandbox");
            if (settings.pickRemoteDebuggingPort && !commandLine->HasSwitch("remote-debugging-port"))
                commandLine->AppendSwitchWithValue("remote-debugging-port", "0");
#ifndef __APPLE__
            if (!commandLine->HasSwitch("ozone-platform"))
            {
                if (auto const* backend = std::getenv("GDK_BACKEND"); backend != nullptr)
                {
                    const std::string_view value{backend};
                    if (value.starts_with("x11"))
                        commandLine->AppendSwitchWithValue("ozone-platform", "x11");
                    else if (value.starts_with("wayland"))
                        commandLine->AppendSwitchWithValue("ozone-platform", "wayland");
                }
            }
#endif
        }

        class BrowserProcess : public CefBrowserProcessHandler
        {
          public:
            explicit BrowserProcess(std::vector<std::string> customSchemes)
                : customSchemes_{std::move(customSchemes)}
            {}

            void OnContextInitialized() override
            {
                // Like the native engines, no spell checking, which would download dictionaries.
                const auto context = CefRequestContext::GetGlobalContext();
                CefString error;
                auto disabled = CefValue::Create();
                disabled->SetBool(false);
                context->SetPreference("browser.enable_spellchecking", disabled, error);
                auto noDictionaries = CefValue::Create();
                noDictionaries->SetList(CefListValue::Create());
                context->SetPreference("spellcheck.dictionaries", noDictionaries, error);
            }

            void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> commandLine) override
            {
                // Helpers are separate executables, they get the scheme names from here.
                commandLine->AppendSwitchWithValue(customSchemesSwitch, joinList(customSchemes_, ','));
            }

            bool OnAlreadyRunningAppRelaunch(CefRefPtr<CefCommandLine>, CefString const&) override
            {
                // Every process has its own cache slot, so this only happens with a foreign root_cache_path. Returning
                // false would open a Chrome browser window.
                return true;
            }

          private:
            std::vector<std::string> customSchemes_;
            IMPLEMENT_REFCOUNTING(BrowserProcess);
        };

        enum class Phase
        {
            Idle,
            Started,
            Running,
            Finished,
            Failed,
        };

        void exitHook();
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool isMainThread()
    {
#ifdef __APPLE__
        return ::pthread_main_np() != 0;
#else
        return ::gettid() == ::getpid();
#endif
    }
    // #####################################################################################################################
    struct EngineState::Implementation
    {
        mutable std::mutex mutex{};
        EngineOptions options{};
        bool engineAttached{false};
        std::vector<std::string> customSchemes{};
        std::set<std::string> htmlAuthorities{};
        bool anyDebugWindow{false};

        Phase phase{Phase::Idle};
        std::string startError{};
        std::thread::id uiThread{};
        int openWindows{0};
        int keepAlives{0};
        std::vector<std::function<void()>> pendingBrowserCreations{};
        std::vector<std::function<void()>> preStartTasks{};
        std::unordered_map<int, std::weak_ptr<WindowShared>> browsers{};

        std::optional<InstanceSlot> slot{};
        std::string desktopApplicationId{};
        std::unique_ptr<boost::asio::thread_pool> readerPool{};
        CefRefPtr<CefApp> app{};
    };
    //---------------------------------------------------------------------------------------------------------------------
    EngineState& EngineState::instance()
    {
        static EngineState state;
        return state;
    }
    //---------------------------------------------------------------------------------------------------------------------
    EngineState::EngineState()
        : impl_{std::make_unique<Implementation>()}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    EngineState::~EngineState() = default;
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::attachEngine(EngineOptions options)
    {
        std::scoped_lock lock{impl_->mutex};
        if (impl_->engineAttached)
            throw std::logic_error("Nui: only one Nui::Engine may exist");
        if (impl_->phase != Phase::Idle)
            throw std::logic_error("Nui: the engine already started, create the Nui::Engine before the first Window::run");
        impl_->engineAttached = true;
        for (auto const& scheme : options.customSchemes)
        {
            if (std::ranges::find(impl_->customSchemes, scheme) == impl_->customSchemes.end())
                impl_->customSchemes.push_back(scheme);
        }
        // Browser arguments of windows constructed before the Engine are kept.
        options.browserArguments.insert(
            options.browserArguments.end(), impl_->options.browserArguments.begin(), impl_->options.browserArguments.end());
        impl_->options = std::move(options);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::detachEngine()
    {
        bool shutDown = false;
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->engineAttached = false;
            shutDown = impl_->phase == Phase::Started && impl_->openWindows == 0 && isMainThread();
        }
        if (shutDown)
            shutdown();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::registerWindowRequirements(
        std::vector<std::string> const& schemes,
        std::string const& htmlAuthority,
        bool debug,
        std::optional<std::string> const& browserArguments)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->phase == Phase::Finished)
                throw std::logic_error(
                    "Nui: the CEF engine has finished, its message loop runs once per process. Create the next window "
                    "before the last one closes.");
            impl_->anyDebugWindow = impl_->anyDebugWindow || debug;
            if (browserArguments && !browserArguments->empty())
            {
                if (impl_->phase == Phase::Idle)
                {
                    for (auto& argument : tokenizeArguments(*browserArguments))
                        impl_->options.browserArguments.push_back(std::move(argument));
                }
                else
                    std::cerr << "Nui: WindowOptions::browserArguments of a window created after the engine started "
                                 "are ignored"
                              << std::endl;
            }
            for (auto const& scheme : schemes)
            {
                if (std::ranges::find(impl_->customSchemes, scheme) != impl_->customSchemes.end())
                    continue;
                if (impl_->phase != Phase::Idle)
                    throw std::logic_error(
                        fmt::format(
                            "Nui: custom scheme '{}' was not registered before the CEF engine started, add it to "
                            "EngineOptions::customSchemes",
                            scheme));
                impl_->customSchemes.push_back(scheme);
            }
        }
        ensureHtmlAuthority(htmlAuthority);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::ensureHtmlAuthority(std::string const& authority)
    {
        bool registerNow = false;
        {
            std::scoped_lock lock{impl_->mutex};
            if (!impl_->htmlAuthorities.insert(authority).second)
                return;
            registerNow = impl_->phase == Phase::Started || impl_->phase == Phase::Running;
        }
        if (registerNow)
            CefRegisterSchemeHandlerFactory("https", authority, new HtmlHandlerFactory());
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool EngineState::started() const
    {
        std::scoped_lock lock{impl_->mutex};
        return impl_->phase == Phase::Started || impl_->phase == Phase::Running;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool EngineState::loopRunning() const
    {
        std::scoped_lock lock{impl_->mutex};
        return impl_->phase == Phase::Running;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool EngineState::finished() const
    {
        std::scoped_lock lock{impl_->mutex};
        return impl_->phase == Phase::Finished;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::start()
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->phase == Phase::Failed)
                throw std::runtime_error(impl_->startError);
            if (impl_->phase == Phase::Finished)
                throw std::logic_error("Nui: the CEF engine has finished and cannot start again in this process");
            if (impl_->phase != Phase::Idle)
                return;
        }
        if (!isMainThread())
            throw std::logic_error("Nui: the CEF engine must start on the main thread");

        try
        {
#ifdef __APPLE__
            prepareMacApplication();
#endif
            if (cef_api_hash(CEF_API_VERSION, 0) == nullptr)
            {
                throw std::runtime_error(
                    fmt::format(
                        "Nui: libcef {} does not support CEF API version {} that Nui was built with",
                        loadedCefVersion(),
                        CEF_API_VERSION));
            }

            EngineOptions options;
            std::vector<std::string> customSchemes;
            std::set<std::string> htmlAuthorities;
            bool anyDebugWindow = false;
            {
                std::scoped_lock lock{impl_->mutex};
                options = impl_->options;
                customSchemes = impl_->customSchemes;
                htmlAuthorities = impl_->htmlAuthorities;
                anyDebugWindow = impl_->anyDebugWindow;
            }

            const auto executable = executablePath();
            const auto applicationId = options.applicationId.value_or(executable.filename().string());
            const auto cacheRoot = options.cacheRoot.value_or(userCacheDirectory()) / applicationId / "cef";
            std::string slotError;
            auto slot = InstanceSlot::acquire(cacheRoot, loadedCefVersion(), slotError);
            if (!slot)
                throw std::runtime_error("Nui: no CEF cache directory: " + slotError);

            CommandLineSettings commandLineSettings{.browserArguments = options.browserArguments};
            if (auto const* extra = std::getenv("NUI_CEF_ARGUMENTS"); extra != nullptr)
            {
                for (auto& argument : tokenizeArguments(extra))
                    commandLineSettings.browserArguments.push_back(std::move(argument));
            }

            CefSettings settings{};
            settings.multi_threaded_message_loop = false;
            settings.disable_signal_handlers = true;
            settings.log_severity = LOGSEVERITY_WARNING;
            CefString(&settings.root_cache_path) = slot->directory().string();
            CefString(&settings.log_file) = (slot->directory() / "debug.log").string();

            auto remoteDebuggingPort = options.remoteDebuggingPort;
            if (auto const* port = std::getenv("NUI_CEF_REMOTE_DEBUGGING_PORT"); port != nullptr && anyDebugWindow)
                remoteDebuggingPort = std::atoi(port);
            if (remoteDebuggingPort && *remoteDebuggingPort > 0)
                settings.remote_debugging_port = *remoteDebuggingPort;
            commandLineSettings.pickRemoteDebuggingPort = remoteDebuggingPort && *remoteDebuggingPort == 0;

#ifndef __APPLE__
            const auto helper = findHelper(options);
            CefString(&settings.browser_subprocess_path) = helper.string();
            const auto libcef = libcefDirectory().value_or(helper.parent_path());
            SandboxDecision sandbox{};
            if (options.sandbox == SandboxPolicy::Disabled)
                sandbox = {.disable = true, .reason = "disabled by EngineOptions::sandbox"};
            else
                sandbox = probeSandbox(libcef);
            if (sandbox.disable && options.sandbox == SandboxPolicy::Required)
                throw std::runtime_error("Nui: Chromium's sandbox is not available: " + sandbox.reason);
            if (sandbox.disable && options.sandbox == SandboxPolicy::Auto)
                std::cerr << "Nui: running CEF without sandbox, " << sandbox.reason << std::endl;
            commandLineSettings.disableSandbox = sandbox.disable;
            settings.no_sandbox = sandbox.disable;
#else
            settings.no_sandbox = options.sandbox == SandboxPolicy::Disabled;
#endif

            impl_->app = new App(
                customSchemes,
                new BrowserProcess(customSchemes),
                nullptr,
                [commandLineSettings](CefString const& processType, CefRefPtr<CefCommandLine> commandLine) {
                    if (processType.empty())
                        adjustBrowserCommandLine(commandLine, commandLineSettings);
                });

            const SignalSnapshot signals;
            if (signals.childSignalIgnored())
                throw std::runtime_error("Nui: SIGCHLD is ignored, Chromium cannot track its child processes then");

            auto& arguments = processArguments(options.passProcessArguments);
            const CefMainArgs mainArgs{static_cast<int>(arguments.storage.size()), arguments.pointers.data()};
            const bool initialized = CefInitialize(mainArgs, settings, impl_->app, nullptr);
            signals.restore();
            if (!initialized)
                throw std::runtime_error(fmt::format("Nui: CefInitialize failed with exit code {}", CefGetExitCode()));

            for (auto const& scheme : customSchemes)
                CefRegisterSchemeHandlerFactory(scheme, "", new SchemeHandlerFactory(scheme));
            for (auto const& authority : htmlAuthorities)
                CefRegisterSchemeHandlerFactory("https", authority, new HtmlHandlerFactory());

            std::vector<std::function<void()>> preStartTasks;
            {
                std::scoped_lock lock{impl_->mutex};
                impl_->slot = std::move(slot);
#ifndef __APPLE__
                impl_->desktopApplicationId = desktopIdentity(options);
#endif
                impl_->readerPool = std::make_unique<boost::asio::thread_pool>(4);
                impl_->uiThread = std::this_thread::get_id();
                impl_->phase = Phase::Started;
                preStartTasks = std::move(impl_->preStartTasks);
                impl_->preStartTasks.clear();
                // Windows constructed before the start with schemes the start did not know yet cannot exist, the
                // registration list was complete when it was copied above.
            }
            for (auto& task : preStartTasks)
                Detail::Cef::postToUi(std::move(task));
            std::atexit(&exitHook);
        }
        catch (std::exception const& exception)
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->phase = Phase::Failed;
            impl_->startError = exception.what();
            throw;
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::runLoop()
    {
        start();
        std::vector<std::function<void()>> creations;
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->phase == Phase::Running)
            {
                std::cerr << "Nui: Window::run called while the engine runs, it returns at once" << std::endl;
                return;
            }
            if (impl_->openWindows == 0 && impl_->keepAlives == 0)
                return;
            impl_->phase = Phase::Running;
            creations = std::move(impl_->pendingBrowserCreations);
            impl_->pendingBrowserCreations.clear();
        }
        for (auto& create : creations)
        {
#ifdef __APPLE__
            withAutoreleasePool(create);
#else
            create();
#endif
        }

        CefRunMessageLoop();

        {
            std::scoped_lock lock{impl_->mutex};
            impl_->phase = Phase::Finished;
        }
        shutdown();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::shutdown()
    {
        if (impl_->readerPool)
        {
            impl_->readerPool->stop();
            impl_->readerPool->join();
        }
        {
            std::scoped_lock lock{impl_->mutex};
            impl_->browsers.clear();
            impl_->phase = Phase::Finished;
        }
        impl_->app = nullptr;
        CefShutdown();
        impl_->slot.reset();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::shutdownAtExit()
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->phase != Phase::Started)
                return;
            if (impl_->openWindows != 0)
            {
                std::cerr << "Nui: process exit with open windows, CEF is not shut down" << std::endl;
                return;
            }
        }
        shutdown();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::windowOpened(std::function<void()> createBrowser)
    {
        bool postNow = false;
        {
            std::scoped_lock lock{impl_->mutex};
            ++impl_->openWindows;
            if (impl_->phase == Phase::Running)
                postNow = true;
            else
                impl_->pendingBrowserCreations.push_back(std::move(createBrowser));
        }
        if (postNow)
            Detail::Cef::postToUi(std::move(createBrowser));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::windowClosed()
    {
        {
            std::scoped_lock lock{impl_->mutex};
            --impl_->openWindows;
        }
        quitIfIdle();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::closeAllWindows()
    {
        std::vector<std::shared_ptr<WindowShared>> windows;
        {
            std::scoped_lock lock{impl_->mutex};
            for (auto const& [identifier, weak] : impl_->browsers)
            {
                if (auto window = weak.lock())
                    windows.push_back(std::move(window));
            }
        }
        for (auto const& window : windows)
        {
            if (window->requestClose)
                window->requestClose();
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::addKeepAlive()
    {
        std::scoped_lock lock{impl_->mutex};
        ++impl_->keepAlives;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::removeKeepAlive()
    {
        {
            std::scoped_lock lock{impl_->mutex};
            --impl_->keepAlives;
        }
        quitIfIdle();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::quitIfIdle()
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->phase != Phase::Running || impl_->openWindows != 0 || impl_->keepAlives != 0)
                return;
        }
        // CefQuitMessageLoop must run on the UI thread, and the check is repeated there.
        Detail::Cef::postToUi([]() {
            auto& state = EngineState::instance();
            {
                std::scoped_lock lock{state.impl_->mutex};
                if (state.impl_->phase != Phase::Running || state.impl_->openWindows != 0 ||
                    state.impl_->keepAlives != 0)
                    return;
            }
            CefQuitMessageLoop();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::registerBrowser(int browserId, std::shared_ptr<WindowShared> window)
    {
        std::scoped_lock lock{impl_->mutex};
        impl_->browsers[browserId] = std::move(window);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::unregisterBrowser(int browserId)
    {
        std::scoped_lock lock{impl_->mutex};
        impl_->browsers.erase(browserId);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<WindowShared> EngineState::windowOf(int browserId, std::string const& scheme) const
    {
        std::scoped_lock lock{impl_->mutex};
        if (const auto iter = impl_->browsers.find(browserId); iter != impl_->browsers.end())
            return iter->second.lock();
        // Requests without a browser (shared and service workers): the only window that can serve them.
        std::shared_ptr<WindowShared> candidate;
        for (auto const& [identifier, weak] : impl_->browsers)
        {
            auto window = weak.lock();
            if (!window || (!scheme.empty() && !window->schemes.contains(scheme)))
                continue;
            if (candidate)
                return nullptr;
            candidate = std::move(window);
        }
        return candidate;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void EngineState::postToUi(std::function<void()> function)
    {
        {
            std::scoped_lock lock{impl_->mutex};
            if (impl_->phase == Phase::Idle)
            {
                impl_->preStartTasks.push_back(std::move(function));
                return;
            }
            if (impl_->phase != Phase::Started && impl_->phase != Phase::Running)
                return;
        }
        Detail::Cef::postToUi(std::move(function));
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool EngineState::onUiThread() const
    {
        std::scoped_lock lock{impl_->mutex};
        if (impl_->phase == Phase::Started || impl_->phase == Phase::Running)
            return std::this_thread::get_id() == impl_->uiThread;
        return isMainThread();
    }
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::thread_pool& EngineState::readerPool()
    {
        return *impl_->readerPool;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<int> EngineState::remoteDebuggingPort() const
    {
        std::scoped_lock lock{impl_->mutex};
        if (!impl_->slot)
            return std::nullopt;
        std::ifstream file{impl_->slot->directory() / "DevToolsActivePort"};
        int port = 0;
        if (file >> port && port > 0)
            return port;
        if (impl_->options.remoteDebuggingPort && *impl_->options.remoteDebuggingPort > 0)
            return impl_->options.remoteDebuggingPort;
        return std::nullopt;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string const& EngineState::desktopApplicationId() const
    {
        return impl_->desktopApplicationId;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::vector<std::string> const& EngineState::customSchemes() const
    {
        return impl_->customSchemes;
    }
    //---------------------------------------------------------------------------------------------------------------------
    namespace
    {
        void exitHook()
        {
            auto& state = EngineState::instance();
            if (!state.started() || state.loopRunning())
                return;
            if (!isMainThread())
            {
                std::cerr << "Nui: process exit on another thread, CEF is not shut down" << std::endl;
                return;
            }
            state.shutdownAtExit();
        }
    }
}
// #####################################################################################################################
namespace Nui
{
    Engine::KeepAlive::KeepAlive()
    {
        Detail::Cef::EngineState::instance().addKeepAlive();
    }
    //---------------------------------------------------------------------------------------------------------------------
    Engine::KeepAlive::~KeepAlive()
    {
        Detail::Cef::EngineState::instance().removeKeepAlive();
    }
    //---------------------------------------------------------------------------------------------------------------------
    Engine::Engine(EngineOptions options)
    {
        Detail::Cef::EngineState::instance().attachEngine(std::move(options));
    }
    //---------------------------------------------------------------------------------------------------------------------
    Engine::~Engine()
    {
        Detail::Cef::EngineState::instance().detachEngine();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Engine::start()
    {
        Detail::Cef::EngineState::instance().start();
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<int> Engine::remoteDebuggingPort() const
    {
        return Detail::Cef::EngineState::instance().remoteDebuggingPort();
    }
}
