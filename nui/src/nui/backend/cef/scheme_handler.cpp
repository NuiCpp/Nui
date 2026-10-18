#include "scheme_handler.hpp"
#include "engine_state.hpp"

#include <include/cef_parser.h>
#include <include/cef_resource_handler.h>

#include <boost/asio/post.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Nui::Detail::Cef
{
    namespace
    {
        bool equalsIgnoringCase(std::string_view lhs, std::string_view rhs)
        {
            return std::ranges::equal(lhs, rhs, [](char left, char right) {
                return std::tolower(static_cast<unsigned char>(left)) == std::tolower(static_cast<unsigned char>(right));
            });
        }

        std::optional<std::string>
        findHeader(std::unordered_multimap<std::string, std::string> const& headers, std::string_view name)
        {
            const auto iter = std::ranges::find_if(headers, [name](auto const& header) {
                return equalsIgnoringCase(header.first, name);
            });
            if (iter == headers.end())
                return std::nullopt;
            return iter->second;
        }

        void eraseHeader(std::unordered_multimap<std::string, std::string>& headers, std::string_view name)
        {
            std::erase_if(headers, [name](auto const& header) {
                return equalsIgnoringCase(header.first, name);
            });
        }

        std::optional<std::uint64_t> parseNumber(std::string_view text)
        {
            while (!text.empty() && text.front() == ' ')
                text.remove_prefix(1);
            while (!text.empty() && text.back() == ' ')
                text.remove_suffix(1);
            std::uint64_t value = 0u;
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (error != std::errc{} || end != text.data() + text.size() || text.empty())
                return std::nullopt;
            return value;
        }

        /// One "bytes=" range. Multiple ranges are not supported, like in CEF itself.
        struct ByteRange
        {
            std::optional<std::uint64_t> first{};
            std::optional<std::uint64_t> last{};
            /// Set for "bytes=-N", the last N bytes.
            std::optional<std::uint64_t> suffixLength{};
        };

        std::optional<ByteRange> parseRange(std::string_view header)
        {
            constexpr std::string_view prefix = "bytes=";
            if (header.size() < prefix.size() || !equalsIgnoringCase(header.substr(0, prefix.size()), prefix))
                return std::nullopt;
            header.remove_prefix(prefix.size());
            if (header.find(',') != std::string_view::npos)
                return std::nullopt;
            const auto dash = header.find('-');
            if (dash == std::string_view::npos)
                return std::nullopt;
            const auto firstText = header.substr(0, dash);
            const auto lastText = header.substr(dash + 1);
            ByteRange range{};
            if (firstText.find_first_not_of(' ') == std::string_view::npos)
            {
                range.suffixLength = parseNumber(lastText);
                if (!range.suffixLength)
                    return std::nullopt;
                return range;
            }
            range.first = parseNumber(firstText);
            if (!range.first)
                return std::nullopt;
            if (lastText.find_first_not_of(' ') != std::string_view::npos)
            {
                range.last = parseNumber(lastText);
                if (!range.last || *range.last < *range.first)
                    return std::nullopt;
            }
            return range;
        }

        /**
         * @brief Request body from the CefPostData elements. Byte elements are copied once, files are read on demand.
         */
        class RequestBody
        {
          public:
            explicit RequestBody(CefRefPtr<CefRequest> const& request)
            {
                const auto postData = request->GetPostData();
                if (!postData)
                    return;
                CefPostData::ElementVector elements;
                postData->GetElements(elements);
                for (auto const& element : elements)
                {
                    switch (element->GetType())
                    {
                        case PDE_TYPE_BYTES:
                        {
                            std::string bytes(element->GetBytesCount(), '\0');
                            element->GetBytes(bytes.size(), bytes.data());
                            parts_.emplace_back(std::move(bytes));
                            break;
                        }
                        case PDE_TYPE_FILE:
                            parts_.emplace_back(std::filesystem::path{element->GetFile().ToString()});
                            break;
                        default:
                            // Blob and stream bodies arrive without data.
                            unsupported_ = true;
                            break;
                    }
                }
            }

            bool unsupported() const
            {
                return unsupported_;
            }

            std::string const& content()
            {
                if (content_)
                    return *content_;
                content_.emplace();
                for (auto const& part : parts_)
                {
                    if (auto const* bytes = std::get_if<std::string>(&part))
                        content_->append(*bytes);
                    else
                    {
                        std::ifstream file{std::get<std::filesystem::path>(part), std::ios::binary};
                        content_->append(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
                    }
                }
                return *content_;
            }

            std::size_t read(char* buffer, std::size_t size)
            {
                while (partIndex_ < parts_.size())
                {
                    auto& part = parts_[partIndex_];
                    std::size_t count = 0u;
                    if (auto const* bytes = std::get_if<std::string>(&part))
                    {
                        count = std::min(size, bytes->size() - partOffset_);
                        std::memcpy(buffer, bytes->data() + partOffset_, count);
                        partOffset_ += count;
                    }
                    else
                    {
                        if (!file_)
                            file_.emplace(std::get<std::filesystem::path>(part), std::ios::binary);
                        file_->read(buffer, static_cast<std::streamsize>(size));
                        count = static_cast<std::size_t>(file_->gcount());
                    }
                    if (count != 0u)
                        return count;
                    ++partIndex_;
                    partOffset_ = 0u;
                    file_.reset();
                }
                return 0u;
            }

          private:
            std::vector<std::variant<std::string, std::filesystem::path>> parts_{};
            std::optional<std::string> content_{};
            std::size_t partIndex_{0u};
            std::size_t partOffset_{0u};
            std::optional<std::ifstream> file_{};
            bool unsupported_{false};
        };

        CustomSchemeResponse errorResponse(int statusCode, std::string body)
        {
            return CustomSchemeResponse{
                .statusCode = statusCode,
                .headers = {{"Content-Type", "text/plain; charset=utf-8"}},
                .body = std::move(body),
            };
        }

        class ResourceHandler : public CefResourceHandler
        {
          public:
            ResourceHandler(std::shared_ptr<WindowShared> window, std::shared_ptr<CustomScheme const> scheme)
                : window_{std::move(window)}
                , scheme_{std::move(scheme)}
            {}

            ResourceHandler(std::shared_ptr<WindowShared> window, CustomSchemeResponse response)
                : window_{std::move(window)}
                , fixedResponse_{std::move(response)}
            {}

            bool Open(CefRefPtr<CefRequest> request, bool& handleRequest, CefRefPtr<CefCallback> callback) override
            {
                if (!scheme_ || scheme_->handlerThread == SchemeHandlerThread::Any)
                {
                    handleRequest = true;
                    respond(request);
                    return true;
                }

                handleRequest = false;
                EngineState::instance().postToUi(
                    [self = CefRefPtr<ResourceHandler>{this}, request, callback]() {
                        self->respond(request);
                        callback->Continue();
                    });
                return true;
            }

            void GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& responseLength, CefString&) override
            {
                response->SetStatus(statusCode_);
                if (!reasonPhrase_.empty())
                    response->SetStatusText(reasonPhrase_);
                response->SetMimeType(mimeType_);
                if (!charset_.empty())
                    response->SetCharset(charset_);
                CefResponse::HeaderMap headerMap;
                for (auto const& [key, value] : headers_)
                    headerMap.emplace(key, value);
                response->SetHeaderMap(headerMap);
                responseLength = responseLength_;
            }

            bool Skip(int64_t bytesToSkip, int64_t& bytesSkipped, CefRefPtr<CefResourceSkipCallback>) override
            {
                if (handlerRanged_)
                {
                    // The handler answered the range itself, its body already starts at the requested offset.
                    bytesSkipped = bytesToSkip;
                    return true;
                }
                if (unsatisfiable_)
                {
                    bytesSkipped = bytesToSkip;
                    return true;
                }
                const auto count = static_cast<std::uint64_t>(bytesToSkip);
                if (file_)
                {
                    file_->seekg(static_cast<std::streamoff>(count), std::ios::cur);
                    if (!file_->good())
                    {
                        bytesSkipped = ERR_FAILED;
                        return false;
                    }
                }
                else if (reader_)
                {
                    std::vector<char> discard(std::min<std::uint64_t>(count, 64u * 1024u));
                    std::uint64_t skipped = 0u;
                    while (skipped < count)
                    {
                        const auto chunk = std::min<std::uint64_t>(discard.size(), count - skipped);
                        const auto read = callReader(discard.data(), static_cast<std::size_t>(chunk));
                        if (read <= 0)
                            break;
                        skipped += static_cast<std::uint64_t>(read);
                    }
                    bytesSkipped = static_cast<int64_t>(skipped);
                    return skipped == count;
                }
                else
                {
                    memoryOffset_ = std::min<std::size_t>(memoryOffset_ + count, memoryBody_ ? memoryBody_->size() : 0u);
                }
                bytesSkipped = bytesToSkip;
                return true;
            }

            bool Read(void* dataOut, int bytesToRead, int& bytesRead, CefRefPtr<CefResourceReadCallback> callback)
                override
            {
                auto* destination = static_cast<char*>(dataOut);
                auto capacity = static_cast<std::size_t>(bytesToRead);
                if (remaining_)
                    capacity = static_cast<std::size_t>(std::min<std::uint64_t>(capacity, *remaining_));
                if (capacity == 0u || unsatisfiable_ || canceled_)
                {
                    bytesRead = 0;
                    return false;
                }

                if (reader_)
                {
                    bytesRead = 0;
                    boost::asio::post(
                        EngineState::instance().readerPool(),
                        [self = CefRefPtr<ResourceHandler>{this}, destination, capacity, callback]() {
                            const auto read = self->callReader(destination, capacity);
                            if (read > 0 && self->remaining_)
                                *self->remaining_ -= static_cast<std::uint64_t>(read);
                            callback->Continue(read);
                        });
                    return true;
                }

                std::size_t count = 0u;
                if (file_)
                {
                    file_->read(destination, static_cast<std::streamsize>(capacity));
                    count = static_cast<std::size_t>(file_->gcount());
                }
                else if (memoryBody_)
                {
                    count = std::min(capacity, memoryBody_->size() - memoryOffset_);
                    std::memcpy(destination, memoryBody_->data() + memoryOffset_, count);
                    memoryOffset_ += count;
                }
                if (remaining_)
                    *remaining_ -= count;
                bytesRead = static_cast<int>(count);
                return count != 0u;
            }

            void Cancel() override
            {
                canceled_ = true;
            }

            /**
             * @brief Body of a fixed response, shared instead of copied.
             */
            void setMemoryBody(std::shared_ptr<std::string const> body)
            {
                fixedBody_ = std::move(body);
            }

          private:
            /// Reads from the body reader of the handler. Returns the byte count, 0 at the end, a net error below 0.
            int callReader(char* buffer, std::size_t size)
            {
                GateEntry entry{window_->gate};
                if (!entry || canceled_)
                    return ERR_ABORTED;
                try
                {
                    return static_cast<int>(reader_(buffer, size));
                }
                catch (std::exception const& exception)
                {
                    std::cerr << "Nui: bodyReader of a custom scheme threw: " << exception.what() << std::endl;
                    return ERR_FAILED;
                }
            }

            CustomSchemeResponse callHandler(CefRefPtr<CefRequest> const& request)
            {
                if (fixedResponse_)
                    return std::move(*fixedResponse_);

                GateEntry entry{window_->gate};
                if (!entry)
                    return errorResponse(404, "Window is closed");

                CefRequest::HeaderMap headerMap;
                request->GetHeaderMap(headerMap);
                std::unordered_multimap<std::string, std::string> headers;
                for (auto const& [key, value] : headerMap)
                    headers.emplace(key.ToString(), value.ToString());

                auto body = std::make_shared<RequestBody>(request);
                if (body->unsupported())
                {
                    std::cerr << "Nui: request to " << request->GetURL().ToString()
                              << " has a body type CEF does not pass to custom schemes (Blob or stream)" << std::endl;
                    return errorResponse(500, "Unsupported request body");
                }

                CustomSchemeRequest customRequest{
                    .scheme = scheme_->scheme,
                    .getContent = std::function<std::string const&()>{},
                    .readContent = {},
                    .headers = std::move(headers),
                    .uri = request->GetURL().ToString(),
                    .method = request->GetMethod().ToString(),
                };
                if (scheme_->streamingContent)
                {
                    customRequest.readContent = [body](char* buffer, std::size_t size) {
                        return body->read(buffer, size);
                    };
                }
                else
                {
                    customRequest.getContent = std::function<std::string const&()>{[body]() -> std::string const& {
                        return body->content();
                    }};
                }

                try
                {
                    return scheme_->onRequest(customRequest);
                }
                catch (std::exception const& exception)
                {
                    return errorResponse(500, exception.what());
                }
            }

            void respond(CefRefPtr<CefRequest> const& request)
            {
                auto response = callHandler(request);

                std::optional<std::uint64_t> totalSize{};
                if (response.bodyFile)
                {
                    std::error_code error;
                    const auto size = std::filesystem::file_size(*response.bodyFile, error);
                    file_.emplace(*response.bodyFile, std::ios::binary);
                    if (error || !file_->good())
                    {
                        file_.reset();
                        response = errorResponse(500, "Could not open " + response.bodyFile->string());
                    }
                    else
                        totalSize = size;
                }
                if (!file_ && response.bodyReader)
                {
                    reader_ = std::move(response.bodyReader);
                    if (const auto length = findHeader(response.headers, "Content-Length"))
                        totalSize = parseNumber(*length);
                }
                else if (!file_)
                {
                    memoryBody_ = fixedBody_ ? std::move(fixedBody_)
                                             : std::make_shared<std::string const>(std::move(response.body));
                    totalSize = memoryBody_->size();
                }

                statusCode_ = response.statusCode;
                reasonPhrase_ = std::move(response.reasonPhrase);
                headers_ = std::move(response.headers);

                auto contentType = findHeader(headers_, "Content-Type").value_or("application/octet-stream");
                if (const auto semicolon = contentType.find(';'); semicolon != std::string::npos)
                {
                    const auto parameters = contentType.substr(semicolon + 1);
                    contentType.resize(semicolon);
                    if (const auto charset = parameters.find("charset="); charset != std::string::npos)
                        charset_ = parameters.substr(charset + 8);
                }
                mimeType_ = contentType;
                eraseHeader(headers_, "Content-Type");
                eraseHeader(headers_, "Content-Length");

                if (scheme_ && !scheme_->allowedOrigins.empty() && !findHeader(headers_, "Access-Control-Allow-Origin"))
                    headers_.emplace("Access-Control-Allow-Origin", scheme_->allowedOrigins.front());

                responseLength_ = totalSize ? static_cast<int64_t>(*totalSize) : -1;
                applyRange(request, totalSize);
            }

            void applyRange(CefRefPtr<CefRequest> const& request, std::optional<std::uint64_t> totalSize)
            {
                if (statusCode_ == 206 || findHeader(headers_, "Content-Range"))
                {
                    handlerRanged_ = true;
                    return;
                }
                const auto rangeHeader = request->GetHeaderByName("Range").ToString();
                if (statusCode_ != 200 || rangeHeader.empty())
                    return;
                const auto range = parseRange(rangeHeader);
                if (!range || !totalSize)
                    return;

                const auto total = *totalSize;
                std::uint64_t first = 0u;
                std::uint64_t last = total == 0u ? 0u : total - 1u;
                if (range->suffixLength)
                {
                    const auto length = std::min(*range->suffixLength, total);
                    first = total - length;
                    // CEF only skips ranges with a first byte, a suffix range is positioned here.
                    if (file_)
                        file_->seekg(static_cast<std::streamoff>(first));
                    else if (memoryBody_)
                        memoryOffset_ = static_cast<std::size_t>(first);
                    else
                        return;
                    if (length == 0u)
                    {
                        unsatisfiable(total);
                        return;
                    }
                }
                else
                {
                    first = *range->first;
                    if (first >= total)
                    {
                        unsatisfiable(total);
                        return;
                    }
                    if (range->last)
                        last = std::min(*range->last, total - 1u);
                }

                statusCode_ = 206;
                reasonPhrase_ = "Partial Content";
                remaining_ = last - first + 1u;
                responseLength_ = static_cast<int64_t>(*remaining_);
                headers_.emplace("Content-Range", fmt::format("bytes {}-{}/{}", first, last, total));
                if (!findHeader(headers_, "Accept-Ranges"))
                    headers_.emplace("Accept-Ranges", "bytes");
            }

            void unsatisfiable(std::uint64_t total)
            {
                unsatisfiable_ = true;
                statusCode_ = 416;
                reasonPhrase_ = "Range Not Satisfiable";
                responseLength_ = 0;
                headers_.emplace("Content-Range", fmt::format("bytes */{}", total));
            }

            std::shared_ptr<WindowShared> window_;
            std::shared_ptr<CustomScheme const> scheme_{};
            std::optional<CustomSchemeResponse> fixedResponse_{};
            std::shared_ptr<std::string const> fixedBody_{};

            int statusCode_{500};
            std::string reasonPhrase_{};
            std::string mimeType_{"application/octet-stream"};
            std::string charset_{};
            std::unordered_multimap<std::string, std::string> headers_{};
            int64_t responseLength_{-1};

            std::optional<std::ifstream> file_{};
            std::function<std::size_t(char*, std::size_t)> reader_{};
            std::shared_ptr<std::string const> memoryBody_{};
            std::size_t memoryOffset_{0u};
            std::optional<std::uint64_t> remaining_{};
            bool handlerRanged_{false};
            bool unsatisfiable_{false};
            std::atomic<bool> canceled_{false};

            IMPLEMENT_REFCOUNTING(ResourceHandler);
        };

        CefRefPtr<CefResourceHandler> notFound(std::shared_ptr<WindowShared> window, std::string message)
        {
            return new ResourceHandler(std::move(window), errorResponse(404, std::move(message)));
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    SchemeHandlerFactory::SchemeHandlerFactory(std::string scheme)
        : scheme_{std::move(scheme)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefResourceHandler> SchemeHandlerFactory::Create(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame>,
        CefString const&,
        CefRefPtr<CefRequest>)
    {
        auto window = EngineState::instance().windowOf(browser ? browser->GetIdentifier() : 0, scheme_);
        if (!window)
            return notFound(std::make_shared<WindowShared>(), "No window for this request");
        const auto scheme = window->schemes.find(scheme_);
        if (scheme == window->schemes.end())
            return notFound(std::move(window), "Scheme not registered by this window");
        return new ResourceHandler(window, scheme->second);
    }
    //---------------------------------------------------------------------------------------------------------------------
    CefRefPtr<CefResourceHandler> HtmlHandlerFactory::Create(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame>,
        CefString const&,
        CefRefPtr<CefRequest> request)
    {
        auto window = EngineState::instance().windowOf(browser ? browser->GetIdentifier() : 0, {});
        if (!window)
            return notFound(std::make_shared<WindowShared>(), "No window for this request");

        CefURLParts parts;
        CefParseURL(request->GetURL(), parts);
        const auto host = CefString(&parts.host).ToString();
        const auto path = CefString(&parts.path).ToString();

        std::shared_ptr<std::string const> html;
        {
            std::shared_lock lock{window->htmlGuard};
            if (host != window->htmlAuthority || (path != "/" && path != "/index.html"))
                return notFound(std::move(window), "Not found");
            html = window->html;
        }
        if (!html)
            return notFound(std::move(window), "No page set");

        CustomSchemeResponse response{
            .statusCode = 200,
            .headers = {{"Content-Type", "text/html; charset=utf-8"}},
        };
        CefRefPtr<ResourceHandler> handler = new ResourceHandler(std::move(window), std::move(response));
        handler->setMemoryBody(std::move(html));
        return handler;
    }
}
