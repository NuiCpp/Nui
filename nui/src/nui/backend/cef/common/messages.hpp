#pragma once

#include <include/cef_process_message.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Nui::Detail::Cef
{
    // Renderer to browser: the UTF-8 text of __webview__.post.
    inline constexpr char const* postMessageName = "nui.post";
    // Browser to renderer: calls nui_rpc.frontend[name], payload [uint32 name size][name][json].
    inline constexpr char const* callMessageName = "nui.call";
    // Browser to renderer: UTF-8 script evaluated in the main frame.
    inline constexpr char const* evalMessageName = "nui.eval";
    // Browser to renderer: UTF-8 script, evaluated now and in every later main frame context of the browser.
    inline constexpr char const* addInitScriptMessageName = "nui.addInitScript";

    // extra_info of a new browser: list of init scripts.
    inline constexpr char const* initScriptsKey = "nui.initScripts";
    // extra_info of a new browser: schemes and https hosts whose main frame documents get __webview__.
    inline constexpr char const* appSchemesKey = "nui.appSchemes";
    inline constexpr char const* appHostsKey = "nui.appHosts";

    // Child process switch with the comma separated custom scheme names.
    inline constexpr char const* customSchemesSwitch = "nui-custom-schemes";

    // Payloads from this size on travel in shared memory instead of the message's argument list.
    inline constexpr std::size_t sharedMemoryThreshold = 64u * 1024u;

    /**
     * @brief Creates a message whose payload is the concatenation of the given parts, in shared memory for large
     * payloads and as binary argument otherwise.
     *
     * @param name Message name.
     * @param parts Payload parts.
     * @param count Number of parts.
     */
    CefRefPtr<CefProcessMessage> makePayloadMessage(char const* name, std::string_view const* parts, std::size_t count);

    /**
     * @brief Creates a call message for nui_rpc.frontend[functionName].
     */
    CefRefPtr<CefProcessMessage> makeCallMessage(std::string_view functionName, std::string_view json);

    /**
     * @brief Creates a message carrying one UTF-8 text.
     */
    CefRefPtr<CefProcessMessage> makeTextMessage(char const* name, std::string_view text);

    /**
     * @brief Calls the visitor with the payload of a message created by makePayloadMessage. The view is only valid
     * during the call.
     */
    template <typename VisitorT>
    bool visitPayload(CefRefPtr<CefProcessMessage> const& message, VisitorT&& visitor);

    /**
     * @brief Splits a call payload into function name and JSON text.
     */
    bool splitCallPayload(std::string_view payload, std::string_view& functionName, std::string_view& json);
}

#include "messages.ipp"
