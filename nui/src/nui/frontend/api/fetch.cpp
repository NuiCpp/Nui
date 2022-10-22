#include <nui/frontend/api/fetch.hpp>

#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>
#include <nui/frontend/api/console.hpp>

#include <emscripten/val.h>

namespace Nui
{
    void fetch(std::string const& uri, FetchOptions const& options, std::function<void(FetchResponse const&)> callback)
    {
        Nui::RpcClient::getRemoteCallableWithBackChannel(
            "Nui::fetch", [callback = std::move(callback), options](emscripten::val response) {
                FetchResponse resp;
                Nui::convertFromVal<FetchResponse>(response, resp);
                if (!options.dontDecodeBody)
                    resp.body = emscripten::val::global("atob")(resp.body).as<std::string>();
                callback(resp);
            })(uri, options);
    }
    void fetch(std::string const& uri, std::function<void(FetchResponse const&)> callback)
    {
        fetch(uri, {}, std::move(callback));
    }
}