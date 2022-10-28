#include "fetch.hpp"

#include <nui/shared/api/fetch_options.hpp>
#include <roar/curl/request.hpp>
#include <roar/url/url.hpp>
#include <roar/utility/base64.hpp>

#include <iostream>

namespace Nui
{

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        FetchOptions,
        method,
        headers,
        body,
        verbose,
        followRedirects,
        maxRedirects,
        autoReferer,
        dontDecodeBody,
        verifyPeer,
        verifyHost);
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        FetchResponse,
        curlCode,
        status,
        proxyStatus,
        downloadSize,
        redirectUrl,
        body,
        headers)

    void registerFetch(Nui::RpcHub const& hub)
    {
        hub.registerFunction(
            "Nui::fetch", [&hub](std::string const& responseId, std::string url, FetchOptions const& options) {
                std::string body;
                auto request = Roar::Curl::Request{};
                std::unordered_map<std::string, std::string> headers;

                bool badUrl = false;
                const auto parsedUrl = boost::leaf::try_handle_some(
                    [&url]() {
                        return Roar::Url::fromString(url);
                    },
                    [&badUrl, &url](std::string errString) -> boost::leaf::result<Roar::Url> {
                        std::cerr << "Bad url: (" << url << ")" << errString << "\n";

                        badUrl = true;
                        return boost::leaf::new_error(errString);
                    });
                if (badUrl)
                {
                    hub.callRemote(responseId, nlohmann::json());
                    return;
                }

                request.sink(body)
                    .followRedirects(options.followRedirects)
                    .maxRedirects(static_cast<std::size_t>(options.maxRedirects))
                    .verbose(options.verbose)
                    .setHeaderFields(options.headers)
                    .autoReferer(options.autoReferer)
                    .verifyPeer(options.verifyPeer)
                    .verifyHost(options.verifyHost)
                    .headerSink(headers);
                if (!options.body.empty() && options.method != "GET")
                    request.source(options.body);

                const auto response = request.method("GET").url(parsedUrl.value().toString(true, false)).perform();
                FetchResponse resp{
                    .curlCode = response.result(),
                    .status = static_cast<int>(response.code()),
                    .proxyStatus = static_cast<int>(response.proxyCode()),
                    .downloadSize = static_cast<uint32_t>(response.sizeOfDownload()),
                    .redirectUrl = response.redirectUrl(),
                    .body = Roar::base64Encode(body),
                    .headers = std::move(headers)};

                hub.callRemote(responseId, resp);
            });
    }
}