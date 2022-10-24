#pragma once

#include <nui/shared/api/fetch_options.hpp>

namespace Nui
{
    /**
     * @brief Simplified fetch, that uses curl in the backend to fetch data.
     * This circumvents the need for ASYNCIFY. Downloading big amounts of data with this is not optimal.
     *
     * @param uri URI to fetch.
     * @param options Options for the fetch.
     * @param callback Callback that is called when the fetch is done.
     */
    void fetch(
        std::string const& uri,
        FetchOptions const& options,
        std::function<void(std::optional<FetchResponse> const&)> callback);
    void fetch(std::string const& uri, std::function<void(std::optional<FetchResponse> const&)> callback);
}