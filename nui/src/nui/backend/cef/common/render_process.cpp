#include "render_process.hpp"
#include "app.hpp"
#include "messages.hpp"

#include <include/cef_command_line.h>
#include <include/cef_parser.h>
#include <include/cef_v8.h>

#include <algorithm>
#include <iostream>
#include <string_view>

namespace Nui::Detail::Cef
{
    namespace
    {
        // Runs before the init scripts in every app document. __nui__.callFrontend has the semantics of the native
        // engines' eval based call: missing functions go to nui_rpc.errors, exceptions surface as uncaught errors.
        constexpr char const* bootstrapScript = R"(
            (() => {
                globalThis.nui_rpc = (globalThis.nui_rpc || {
                    frontend: {}, backend: {}, tempId: 0, initialized: false
                });
                globalThis.nui_rpc.engine = "cef";
                Object.defineProperty(globalThis, "__nui__", {
                    value: Object.freeze({
                        callFrontend: (name, json) => {
                            const rpc = globalThis.nui_rpc;
                            if (rpc && rpc.frontend && Object.prototype.hasOwnProperty.call(rpc.frontend, name)) {
                                try {
                                    rpc.frontend[name](json.length === 0 ? undefined : JSON.parse(json));
                                } catch (error) {
                                    setTimeout(() => { throw error; });
                                }
                                return;
                            }
                            if (!rpc)
                                return;
                            rpc.errors = rpc.errors || [];
                            rpc.errors.push("Function " + name + " does not exist.");
                            if (rpc.errors.length > 100)
                                rpc.errors.shift();
                        }
                    })
                });
            })();
        )";

        // Operations for a document that has no script context yet. More are dropped, like calls into a view
        // without a page on the native engines.
        constexpr std::size_t maximumPendingOperations = 10000u;

        CefString toCefString(std::string_view text)
        {
            CefString result;
            cef_string_utf8_to_utf16(text.data(), text.size(), result.GetWritableStruct());
            return result;
        }

        std::vector<std::string> stringList(CefRefPtr<CefDictionaryValue> const& dictionary, char const* key)
        {
            std::vector<std::string> result;
            if (!dictionary || !dictionary->HasKey(key))
                return result;
            const auto list = dictionary->GetList(key);
            for (std::size_t index = 0u; index != list->GetSize(); ++index)
                result.push_back(list->GetString(index).ToString());
            return result;
        }

        class PostHandler : public CefV8Handler
        {
          public:
            bool Execute(
                CefString const&,
                CefRefPtr<CefV8Value>,
                CefV8ValueList const& arguments,
                CefRefPtr<CefV8Value>&,
                CefString& exception) override
            {
                if (arguments.size() != 1u || !arguments[0]->IsString())
                {
                    exception = "__webview__.post expects one string";
                    return true;
                }
                const auto frame = CefV8Context::GetCurrentContext()->GetFrame();
                if (!frame || !frame->IsMain())
                    return true;
                frame->SendProcessMessage(
                    PID_BROWSER, makeTextMessage(postMessageName, arguments[0]->GetStringValue().ToString()));
                return true;
            }

          private:
            IMPLEMENT_REFCOUNTING(PostHandler);
        };

        void evaluate(CefRefPtr<CefV8Context> const& context, std::string const& script, char const* origin)
        {
            // CEF rejects empty code.
            if (script.empty())
                return;
            CefRefPtr<CefV8Value> result;
            CefRefPtr<CefV8Exception> exception;
            if (!context->Eval(script, origin, 1, result, exception) && exception)
            {
                std::cerr << "Nui: " << origin << " failed: " << exception->GetMessage().ToString() << " (line "
                          << exception->GetLineNumber() << ")" << std::endl;
            }
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RenderProcess::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extraInfo)
    {
        // CEF can create the new browser object of a navigation before it destroys the old one in this process, so the
        // state is reference counted and the first creation's state is kept.
        auto& state = browsers_[browser->GetIdentifier()];
        if (state.references++ != 0)
            return;
        const auto scripts = stringList(extraInfo, initScriptsKey);
        for (std::size_t index = 0u; index != scripts.size(); ++index)
            state.initScripts.emplace(static_cast<std::uint32_t>(index), scripts[index]);
        advanceScripts(state);
        state.appSchemes = stringList(extraInfo, appSchemesKey);
        state.appHosts = stringList(extraInfo, appHostsKey);
        // A cross site setHtml navigation to an authority added after the browser was created lands in a new process,
        // which only knows it from its own command line. Renderers forked from the zygote get it there only now.
        for (auto const& host : listFromCommandLine(CefCommandLine::GetGlobalCommandLine(), appHostsSwitch))
        {
            if (std::ranges::find(state.appHosts, host) == state.appHosts.end())
                state.appHosts.push_back(host);
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RenderProcess::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
    {
        const auto iter = browsers_.find(browser->GetIdentifier());
        if (iter != browsers_.end() && --iter->second.references <= 0)
            browsers_.erase(iter);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool RenderProcess::isAppDocument(BrowserState const& state, CefRefPtr<CefFrame> const& frame) const
    {
        CefURLParts parts;
        if (!CefParseURL(frame->GetURL(), parts))
            return false;
        const auto scheme = CefString(&parts.scheme).ToString();
        if (scheme == "file" || std::ranges::find(state.appSchemes, scheme) != state.appSchemes.end())
            return true;
        const auto host = CefString(&parts.host).ToString();
        return scheme == "https" && std::ranges::find(state.appHosts, host) != state.appHosts.end();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RenderProcess::OnContextCreated(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context)
    {
        if (!frame->IsMain())
            return;
        const auto iter = browsers_.find(browser->GetIdentifier());
        if (iter == browsers_.end())
            return;
        auto& state = iter->second;
        // Scripts added after the browser's creation only reached the processes that existed then.
        if (!state.stateRequested)
        {
            state.stateRequested = true;
            frame->SendProcessMessage(PID_BROWSER, CefProcessMessage::Create(stateRequestMessageName));
        }
        if (!isAppDocument(state, frame))
        {
            state.contextReady = false;
            state.pending.clear();
            return;
        }

        auto webview = CefV8Value::CreateObject(nullptr, nullptr);
        webview->SetValue(
            "post", CefV8Value::CreateFunction("post", new PostHandler()), V8_PROPERTY_ATTRIBUTE_READONLY);
        context->GetGlobal()->SetValue("__webview__", webview, V8_PROPERTY_ATTRIBUTE_READONLY);

        evaluate(context, bootstrapScript, "nui://bootstrap");
        for (auto const& [index, script] : state.initScripts)
        {
            if (index >= state.contiguousScripts)
                break;
            evaluate(context, script, "nui://init-script");
        }

        state.contextReady = true;
        auto pending = std::move(state.pending);
        state.pending.clear();
        for (auto const& operation : pending)
            runOperation(frame, operation.name, operation.payload);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::vector<std::string const*> RenderProcess::advanceScripts(BrowserState& state)
    {
        std::vector<std::string const*> runnable;
        for (auto iter = state.initScripts.find(state.contiguousScripts); iter != state.initScripts.end() &&
             iter->first == state.contiguousScripts;
             ++iter, ++state.contiguousScripts)
            runnable.push_back(&iter->second);
        return runnable;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RenderProcess::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context>)
    {
        if (!frame->IsMain())
            return;
        if (const auto iter = browsers_.find(browser->GetIdentifier()); iter != browsers_.end())
            iter->second.contextReady = false;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RenderProcess::runOperation(CefRefPtr<CefFrame> const& frame, std::string const& name, std::string_view payload)
    {
        if (name == evalMessageName)
        {
            if (!payload.empty())
                frame->ExecuteJavaScript(toCefString(payload), frame->GetURL(), 0);
            return;
        }
        if (name != callMessageName)
            return;

        std::string_view functionName;
        std::string_view json;
        if (!splitCallPayload(payload, functionName, json))
            return;

        const auto context = frame->GetV8Context();
        if (!context || !context->Enter())
            return;
        const auto nui = context->GetGlobal()->GetValue("__nui__");
        const auto callFrontend = nui && nui->IsObject() ? nui->GetValue("callFrontend") : nullptr;
        if (callFrontend && callFrontend->IsFunction())
        {
            callFrontend->ExecuteFunction(
                nullptr, {CefV8Value::CreateString(toCefString(functionName)), CefV8Value::CreateString(toCefString(json))});
            if (callFrontend->HasException())
                callFrontend->ClearException();
        }
        context->Exit();
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool RenderProcess::OnProcessMessageReceived(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame>,
        CefProcessId,
        CefRefPtr<CefProcessMessage> message)
    {
        const auto name = message->GetName().ToString();
        if (name != callMessageName && name != evalMessageName && name != addInitScriptMessageName &&
            name != addAppHostMessageName)
            return false;

        const auto iter = browsers_.find(browser->GetIdentifier());
        if (iter == browsers_.end())
            return true;
        auto& state = iter->second;
        const auto frame = browser->GetMainFrame();

        visitPayload(message, [&](std::string_view payload) {
            if (name == addAppHostMessageName)
            {
                if (std::ranges::find(state.appHosts, payload) == state.appHosts.end())
                    state.appHosts.emplace_back(payload);
                return;
            }
            if (name == addInitScriptMessageName)
            {
                std::vector<IndexedScript> scripts;
                splitInitScripts(payload, scripts);
                for (auto const& [index, script] : scripts)
                    state.initScripts.try_emplace(index, script);
                const auto added = advanceScripts(state);
                if (added.empty() || !state.contextReady || !frame)
                    return;
                if (const auto context = frame->GetV8Context(); context && context->Enter())
                {
                    for (auto const* script : added)
                        evaluate(context, *script, "nui://init-script");
                    context->Exit();
                }
                return;
            }
            if (state.contextReady && frame)
                runOperation(frame, name, payload);
            else if (state.pending.size() < maximumPendingOperations)
                state.pending.push_back(PendingOperation{.name = name, .payload = std::string{payload}});
        });
        return true;
    }
}
