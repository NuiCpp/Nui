#include "scheme_handler.hpp"
#include "engine_state.hpp"

#include <include/cef_parser.h>
#include <include/cef_resource_handler.h>

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
#include <utility>
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

        /// Parses a Range header like Chromium does: spaces are ignored, a list with one range is a single range.
        std::optional<ByteRange> parseRange(std::string_view header)
        {
            std::string compact;
            for (const auto character : header)
            {
                if (character != ' ' && character != '\t')
                    compact += character;
            }
            constexpr std::string_view prefix = "bytes=";
            std::string_view rest{compact};
            if (rest.size() < prefix.size() || !equalsIgnoringCase(rest.substr(0, prefix.size()), prefix))
                return std::nullopt;
            rest.remove_prefix(prefix.size());
            std::optional<std::string_view> single;
            while (!rest.empty())
            {
                const auto comma = rest.find(',');
                const auto item = rest.substr(0, comma);
                rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
                if (item.empty())
                    continue;
                if (single)
                    return std::nullopt;
                single = item;
            }
            if (!single)
                return std::nullopt;
            const auto dash = single->find('-');
            if (dash == std::string_view::npos)
                return std::nullopt;
            const auto firstText = single->substr(0, dash);
            const auto lastText = single->substr(dash + 1);
            ByteRange range{};
            if (firstText.empty())
            {
                range.suffixLength = parseNumber(lastText);
                if (!range.suffixLength)
                    return std::nullopt;
                return range;
            }
            range.first = parseNumber(firstText);
            if (!range.first)
                return std::nullopt;
            if (!lastText.empty())
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
                : url_{request->GetURL().ToString()}
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

            std::string const& content()
            {
                throwIfUnsupported();
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
                throwIfUnsupported();
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
            void throwIfUnsupported()
            {
                if (!unsupported_)
                    return;
                if (!std::exchange(loggedUnsupported_, true))
                {
                    std::cerr << "Nui: the handler for " << url_
                              << " reads a request body CEF does not pass to custom schemes (Blob or stream)" << std::endl;
                }
                throw std::runtime_error("Unsupported request body, CEF does not pass Blob or stream bodies to custom schemes");
            }

            std::string url_;
            std::vector<std::variant<std::string, std::filesystem::path>> parts_{};
            std::optional<std::string> content_{};
            std::size_t partIndex_{0u};
            std::size_t partOffset_{0u};
            std::optional<std::ifstream> file_{};
            bool unsupported_{false};
            bool loggedUnsupported_{false};
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
                // CEF drops the mime type, charset and length of error responses, the plain headers arrive.
                if (statusCode_ >= 400)
                {
                    headerMap.emplace("Content-Type", charset_.empty() ? mimeType_ : mimeType_ + "; charset=" + charset_);
                    if (responseLength_ >= 0)
                        headerMap.emplace("Content-Length", std::to_string(responseLength_));
                }
                response->SetHeaderMap(headerMap);
                responseLength = responseLength_;
            }

            bool Skip(int64_t bytesToSkip, int64_t& bytesSkipped, CefRefPtr<CefResourceSkipCallback>) override
            {
                // CEF skips for every request with a Range header it parses, whatever the response. Only a range that
                // Nui turned into a 206 here starts at that offset, every other body is served from its start.
                if (!skipMovesBody_)
                {
                    bytesSkipped = bytesToSkip;
                    return true;
                }
                if (reader_)
                {
                    pendingReaderSkip_ += static_cast<std::uint64_t>(bytesToSkip);
                    bytesSkipped = bytesToSkip;
                    return true;
                }
                if (!skipBody(static_cast<std::uint64_t>(bytesToSkip)))
                {
                    bytesSkipped = ERR_FAILED;
                    return false;
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
                if (capacity == 0u || unsatisfiable_ || headOnly_ || canceled_)
                {
                    bytesRead = 0;
                    return false;
                }

                if (reader_)
                {
                    // The application's reader fills a buffer of the handler: a reader the shutdown gave up on may
                    // return after CEF freed its own buffer.
                    if (readerBuffer_.size() < capacity)
                        readerBuffer_.resize(capacity);
                    const bool posted = EngineState::instance().postReader(
                        [self = CefRefPtr<ResourceHandler>{this}, capacity]() {
                            if (self->pendingReaderSkip_ != 0u &&
                                !self->skipBody(std::exchange(self->pendingReaderSkip_, 0u)))
                                return static_cast<int>(ERR_FAILED);
                            return self->callReader(self->readerBuffer_.data(), capacity);
                        },
                        [self = CefRefPtr<ResourceHandler>{this}, destination, callback](int read) {
                            if (read > 0)
                            {
                                std::memcpy(destination, self->readerBuffer_.data(), static_cast<std::size_t>(read));
                                if (self->remaining_)
                                    *self->remaining_ -= static_cast<std::uint64_t>(read);
                            }
                            callback->Continue(read);
                        });
                    bytesRead = posted ? 0 : static_cast<int>(ERR_ABORTED);
                    return posted;
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
            /// Moves the start of the body forward. Blocking for body readers, only called on reader threads.
            bool skipBody(std::uint64_t count)
            {
                if (file_)
                {
                    file_->seekg(static_cast<std::streamoff>(count), std::ios::cur);
                    return file_->good();
                }
                if (reader_)
                {
                    std::vector<char> discard(static_cast<std::size_t>(std::min<std::uint64_t>(count, 64u * 1024u)));
                    std::uint64_t skipped = 0u;
                    while (skipped < count)
                    {
                        const auto chunk = std::min<std::uint64_t>(discard.size(), count - skipped);
                        const auto read = callReader(discard.data(), static_cast<std::size_t>(chunk));
                        if (read <= 0)
                            return false;
                        skipped += static_cast<std::uint64_t>(read);
                    }
                    return true;
                }
                memoryOffset_ = static_cast<std::size_t>(
                    std::min<std::uint64_t>(memoryOffset_ + count, memoryBody_ ? memoryBody_->size() : 0u));
                return true;
            }

            /// Reads from the body reader of the handler. Returns the byte count, 0 at the end, a net error below 0.
            int callReader(char* buffer, std::size_t size)
            {
                GateEntry entry{window_->gate};
                if (!entry || canceled_)
                    return ERR_ABORTED;
                try
                {
                    const auto count = reader_(buffer, size);
                    if (count <= size)
                        return static_cast<int>(count);
                    std::cerr << "Nui: bodyReader of a custom scheme returned " << count << " bytes for a buffer of "
                              << size << std::endl;
                    return ERR_FAILED;
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
                {
                    auto name = key.ToString();
                    if (equalsIgnoringCase(name, rangeHeaderName))
                        name = "Range";
                    headers.emplace(std::move(name), value.ToString());
                }

                auto body = std::make_shared<RequestBody>(request);

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
                    // The body ends at the declared length, whatever the reader would deliver.
                    remaining_ = totalSize;
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
                headOnly_ = request->GetMethod().ToString() == "HEAD";
                if (statusCode_ == 200 && totalSize && !findHeader(headers_, "Accept-Ranges"))
                    headers_.emplace("Accept-Ranges", "bytes");
                applyRange(request, totalSize);
            }

            void applyRange(CefRefPtr<CefRequest> const& request, std::optional<std::uint64_t> totalSize)
            {
                if (statusCode_ == 206 || findHeader(headers_, "Content-Range"))
                    return;
                // Nui's request handler renames Range so CEF's loader leaves it alone and Nui positions the body here.
                // Without that rename (an application CefRequestHandler) CEF skips to the first byte itself.
                auto rangeHeader = request->GetHeaderByName(rangeHeaderName).ToString();
                const bool ownRange = !rangeHeader.empty();
                if (!ownRange)
                    rangeHeader = request->GetHeaderByName("Range").ToString();
                if (statusCode_ != 200 || rangeHeader.empty() || !totalSize)
                    return;
                const auto range = parseRange(rangeHeader);
                if (!range)
                    return;

                const auto total = *totalSize;
                std::uint64_t first = 0u;
                std::uint64_t last = total == 0u ? 0u : total - 1u;
                if (range->suffixLength)
                {
                    const auto length = std::min(*range->suffixLength, total);
                    if (length == 0u)
                    {
                        unsatisfiable(total);
                        return;
                    }
                    first = total - length;
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

                // CEF only skips ranges with a first byte. Everything else, and every renamed range, is positioned here.
                if (ownRange || range->suffixLength)
                {
                    if (reader_)
                        pendingReaderSkip_ = first;
                    else if (!skipBody(first))
                    {
                        statusCode_ = 500;
                        return;
                    }
                }
                else
                    skipMovesBody_ = true;

                statusCode_ = 206;
                reasonPhrase_ = "Partial Content";
                remaining_ = last - first + 1u;
                responseLength_ = static_cast<int64_t>(*remaining_);
                headers_.emplace("Content-Range", fmt::format("bytes {}-{}/{}", first, last, total));
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
            std::vector<char> readerBuffer_{};
            std::shared_ptr<std::string const> memoryBody_{};
            std::size_t memoryOffset_{0u};
            std::optional<std::uint64_t> remaining_{};
            std::uint64_t pendingReaderSkip_{0u};
            bool skipMovesBody_{false};
            bool unsatisfiable_{false};
            bool headOnly_{false};
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
