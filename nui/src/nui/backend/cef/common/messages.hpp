#pragma once

#include <include/cef_process_message.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Nui::Detail::Cef
{
    // Renderer to browser: the UTF-8 text of __webview__.post.
    inline constexpr char const* postMessageName = "nui.post";
    // Renderer to browser, no payload: a renderer process asks once for the window's current init scripts and app
    // hosts, the extra_info of a browser only has those from its creation.
    inline constexpr char const* stateRequestMessageName = "nui.stateRequest";
    // Browser to renderer: calls nui_rpc.frontend[name], payload [uint32 name size][name][json].
    inline constexpr char const* callMessageName = "nui.call";
    // Browser to renderer: UTF-8 script evaluated in the main frame.
    inline constexpr char const* evalMessageName = "nui.eval";
    // Browser to renderer: init scripts with their index in the window's list, evaluated now and in every later main
    // frame context of the browser. Payload repeated [uint32 index][uint32 size][UTF-8 script].
    inline constexpr char const* addInitScriptMessageName = "nui.addInitScript";
    // Browser to renderer: an https host whose documents get __webview__ (a later setHtml authority).
    inline constexpr char const* addAppHostMessageName = "nui.addAppHost";

    // extra_info of a new browser: list of init scripts.
    inline constexpr char const* initScriptsKey = "nui.initScripts";
    // extra_info of a new browser: schemes and https hosts whose main frame documents get __webview__.
    inline constexpr char const* appSchemesKey = "nui.appSchemes";
    inline constexpr char const* appHostsKey = "nui.appHosts";

    // Child process switch with the comma separated custom scheme names.
    inline constexpr char const* customSchemesSwitch = "nui-custom-schemes";
    // Child process switch with the comma separated setHtml authorities known when the process started.
    inline constexpr char const* appHostsSwitch = "nui-app-hosts";

    // Payloads from this size on travel in shared memory instead of the message's argument list. A shared memory
    // message costs about 45 microseconds more and only pays off from about 1 MiB (measured with CEF 154).
    inline constexpr std::size_t sharedMemoryThreshold = 1024u * 1024u;

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
     * @brief An init script and its position in the window's list.
     */
    struct IndexedScript
    {
        std::uint32_t index;
        std::string_view script;
    };

    /**
     * @brief Creates an addInitScript message.
     */
    CefRefPtr<CefProcessMessage> makeInitScriptsMessage(std::vector<IndexedScript> const& scripts);

    /**
     * @brief Splits an addInitScript payload. False for a malformed payload, the entries before the error were visited.
     */
    bool splitInitScripts(std::string_view payload, std::vector<IndexedScript>& scripts);

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
