#pragma once

#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace Nui::Detail
{
    using RpcCallback = std::function<void(nlohmann::json const&)>;

    /**
     * @brief Script that defines globalThis.nui_rpc.backend[name], which posts an rpc message for the bound function.
     *
     * @param name Name of the bound backend function.
     */
    std::string bindScript(std::string const& name);

    /**
     * @brief Script that removes globalThis.nui_rpc.backend[name].
     *
     * @param name Name of the bound backend function.
     */
    std::string unbindScript(std::string const& name);

    /**
     * @brief Script that calls globalThis.nui_rpc.frontend[name] or records the missing function in nui_rpc.errors.
     *
     * @param name Name of the frontend function.
     * @param jsonArgument Argument as JSON text, empty for no argument.
     */
    std::string callFrontendScript(std::string const& name, std::string const& jsonArgument);

    struct RpcMessageHandlers
    {
        /// Returns the callback bound under the id, or an empty function.
        std::function<RpcCallback(std::string const& id)> findCallback;
        std::function<void()> onRpcAlive;
        std::function<void(std::string_view)> onRpcError;
    };

    /**
     * @brief Parses a message posted by the frontend and calls the bound callback or the alive handler.
     *
     * @param message The JSON text posted through __webview__.post.
     * @param handlers Callback lookup and the error and alive handlers.
     */
    void dispatchRpcMessage(std::string_view message, RpcMessageHandlers const& handlers);
}
