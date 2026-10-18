#include "rpc_message.hpp"

#include <nui/backend/rpc_hub.hpp>

#include <fmt/format.h>

#include <exception>

namespace Nui::Detail
{
    std::string bindScript(std::string const& name)
    {
        return fmt::format(
            R"(
                (() => {{
                    const name = "{}";
                    const id = "{}";
                    globalThis.nui_rpc = (globalThis.nui_rpc || {{
                        frontend: {{}}, backend: {{}}, tempId: 0, initialized: false
                    }});
                    globalThis.nui_rpc.engine = globalThis.nui_rpc.engine || "native";
                    globalThis.nui_rpc.backend[name] = (...args) => {{
                        globalThis.__webview__.post(JSON.stringify({{
                            type: "rpc",
                            name: name,
                            id: id,
                            args: [...args]
                        }}));
                    }};
                }})();
            )",
            name,
            name);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string unbindScript(std::string const& name)
    {
        return fmt::format(
            R"(
                (() => {{
                    const name = "{}";
                    delete globalThis.nui_rpc.backend[name];
                }})();
            )",
            name);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string callFrontendScript(std::string const& name, std::string const& jsonArgument)
    {
        return fmt::format(RpcHub::remoteCallScript, name, jsonArgument.empty() ? "undefined" : jsonArgument);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void dispatchRpcMessage(std::string_view message, RpcMessageHandlers const& handlers)
    {
        try
        {
            const auto object = nlohmann::json::parse(message);
            const auto type = object.find("type");
            if (type != object.end() && type->is_string() && type->get<std::string>() == "rpc_alive")
            {
                if (handlers.onRpcAlive)
                    handlers.onRpcAlive();
                return;
            }

            if (!object.contains("id"))
                return handlers.onRpcError("Message does not contain a callback id!");

            const auto id = object["id"].get<std::string>();
            const auto callback = handlers.findCallback(id);
            if (!callback)
                return handlers.onRpcError("Callback with id " + id + " does not exist!");

            if (!object.contains("args"))
                callback(nlohmann::json{});
            else
                callback(object["args"]);
        }
        catch (std::exception const& exception)
        {
            handlers.onRpcError(
                fmt::format(
                    "Exception in webview message handler for message: {}\nException: {}", message, exception.what()));
        }
    }
}
