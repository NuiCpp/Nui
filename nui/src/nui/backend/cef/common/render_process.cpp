#include "render_process.hpp"
#include "messages.hpp"

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
        state.initScripts = stringList(extraInfo, initScriptsKey);
        state.appSchemes = stringList(extraInfo, appSchemesKey);
        state.appHosts = stringList(extraInfo, appHostsKey);
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
        for (auto const& script : state.initScripts)
            evaluate(context, script, "nui://init-script");

        state.contextReady = true;
        auto pending = std::move(state.pending);
        state.pending.clear();
        for (auto const& operation : pending)
            runOperation(frame, operation.name, operation.payload);
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
        if (name != callMessageName && name != evalMessageName && name != addInitScriptMessageName)
            return false;

        const auto iter = browsers_.find(browser->GetIdentifier());
        if (iter == browsers_.end())
            return true;
        auto& state = iter->second;
        const auto frame = browser->GetMainFrame();

        visitPayload(message, [&](std::string_view payload) {
            if (name == addInitScriptMessageName)
            {
                state.initScripts.emplace_back(payload);
                if (state.contextReady && frame)
                {
                    if (const auto context = frame->GetV8Context(); context && context->Enter())
                    {
                        evaluate(context, state.initScripts.back(), "nui://init-script");
                        context->Exit();
                    }
                }
                return;
            }
            if (state.contextReady && frame)
                runOperation(frame, name, payload);
            else
                state.pending.push_back(PendingOperation{.name = name, .payload = std::string{payload}});
        });
        return true;
    }
}
