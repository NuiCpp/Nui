#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wold-style-cast"
#include <boost/describe.hpp>
#include <boost/mp11/algorithm.hpp>
#pragma clang diagnostic pop

namespace Nui
{
    struct FetchOptions
    {
        std::string method = "GET";
        std::unordered_map<std::string, std::string> headers = {};
        std::string body = "";
    };
    BOOST_DESCRIBE_STRUCT(FetchOptions, (), (method, headers, body));

    struct FetchResponse
    {
        int32_t curlCode;
        int32_t status;
        int32_t proxyStatus;
        uint32_t downloadSize;
        std::string redirectUrl;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
    };
    BOOST_DESCRIBE_STRUCT(FetchResponse, (), (curlCode, status, proxyStatus, downloadSize, redirectUrl, body, headers));
}