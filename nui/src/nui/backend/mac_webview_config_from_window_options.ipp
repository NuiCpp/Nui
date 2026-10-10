#include "mac_helpers/helpers.hpp"
#include "mac_helpers/type_encodings.hpp"
#include "mac_helpers/class.hpp"

namespace Nui::MacOs
{
    // RAII-retained Objective-C reference. Construction sends `retain`, destruction sends `release`.
    // shared_ptr aliasing keeps it cheap to copy across captures.
    inline std::shared_ptr<void> retainObjC(id obj)
    {
        if (!obj)
            return {};
        msg_send<id>(obj, "retain"_sel);
        return std::shared_ptr<void>{obj, [](void* p) noexcept {
                                         if (p)
                                             msg_send<void>(static_cast<id>(p), "release"_sel);
                                     }};
    }

    class NuiSchemeHandler
    {
      public:
        NuiSchemeHandler(Class isa)
            : isa{isa}
            , m_scheme{}
        {}

        static id alloc(id, SEL)
        {
            return ClassWrapper<NuiSchemeHandler>::createInstance("NuiSchemeHandler");
        }

        static id init(id self, SEL, void* scheme)
        {
            reinterpret_cast<NuiSchemeHandler*>(self)->m_scheme = *reinterpret_cast<CustomScheme*>(scheme);
            return self;
        }

        static id new_(id self, SEL _cmd, void* scheme)
        {
            auto inst = alloc(self, _cmd);
            return init(inst, _cmd, scheme);
        }

        static Class registerClass()
        {
            [[maybe_unused]] static bool once = []() {
                auto schemeHandler = ClassWrapper<NuiSchemeHandler>::createNsObjectClassPair("NuiSchemeHandler");
                schemeHandler.addProtocol("WKURLSchemeHandler");
                schemeHandler.addMethod("webView:startURLSchemeTask:", &NuiSchemeHandler::startURLSchemeTask);
                schemeHandler.addMethod("webView:stopURLSchemeTask:", &NuiSchemeHandler::stopURLSchemeTask);

                auto staticClass = ClassWrapper<void>{object_getClass(reinterpret_cast<id>(schemeHandler.native()))};
                staticClass.addMethod("alloc", &NuiSchemeHandler::alloc);
                staticClass.addMethod("init", &NuiSchemeHandler::init);
                staticClass.addMethod("new", &NuiSchemeHandler::new_);

                schemeHandler.registerClassPair();
                return true;
            }();

            return reinterpret_cast<Class>("NuiSchemeHandler"_cls);
        }

        static void unregisterClass()
        {
            objc_disposeClassPair(objc_getClass("NuiSchemeHandler"));
        }

        CustomScheme const& scheme() const noexcept
        {
            return m_scheme;
        }

        static void startURLSchemeTask(id self, SEL /*_cmd*/, id /*webView*/, id task)
        {
            webview::detail::objc::autoreleasepool pool;
            // Exceptions must not cross into the Obj-C runtime. On failure, fail the task explicitly so
            // the renderer doesn't hang waiting for didFinish.
            try
            {
                startURLSchemeTaskImpl(self, task);
            }
            catch (std::exception const& ex)
            {
                id domain = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, "NuiSchemeHandler");
                id desc = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, ex.what());
                id userInfo = msg_send<id>("NSDictionary"_cls, "dictionaryWithObject:forKey:"_sel,
                    desc, "NSLocalizedDescription"_str);
                id err = msg_send<id>("NSError"_cls, "errorWithDomain:code:userInfo:"_sel,
                    domain, static_cast<NSInteger>(-1), userInfo);
                msg_send<void>(task, "didFailWithError:"_sel, err);
            }
            catch (...)
            {
                id domain = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, "NuiSchemeHandler");
                id desc = "Unknown C++ exception in custom scheme handler"_str;
                id userInfo = msg_send<id>("NSDictionary"_cls, "dictionaryWithObject:forKey:"_sel,
                    desc, "NSLocalizedDescription"_str);
                id err = msg_send<id>("NSError"_cls, "errorWithDomain:code:userInfo:"_sel,
                    domain, static_cast<NSInteger>(-1), userInfo);
                msg_send<void>(task, "didFailWithError:"_sel, err);
            }
        }

        static void startURLSchemeTaskImpl(id self, id task)
        {
            NuiSchemeHandler* handler = reinterpret_cast<NuiSchemeHandler*>(self);

            id request = msg_send<id>(task, "request"_sel);
            id method = msg_send<id>(request, "HTTPMethod"_sel);
            std::string methodStr = msg_send<const char*>(method, "UTF8String"_sel);
            id url = msg_send<id>(request, "URL"_sel);

            const bool streaming = handler->scheme().streamingContent;

            // Hold a strong ref on the request so body-reading lambdas stay safe even if a user copies
            // them out of the synchronous onRequest scope (the API allows it; we don't want UAF when they do).
            auto requestRef = retainObjC(request);

            CustomSchemeRequest schemeRequest = {
                .scheme = handler->scheme().scheme,
                .getContent = streaming ? std::function<std::string()>{} : std::function<std::string()>{[requestRef]() {
                    id body = msg_send<id>(static_cast<id>(requestRef.get()), "HTTPBody"_sel);
                    if (!body)
                        return std::string{};
                    const NSUInteger length = msg_send<NSUInteger>(body, "length"_sel);
                    const void* bytes = msg_send<const void*>(body, "bytes"_sel);
                    if (!bytes || length == 0)
                        return std::string{};
                    return std::string(static_cast<const char*>(bytes), static_cast<std::size_t>(length));
                }},
                .readContent = streaming
                    ? std::function<std::size_t(char*, std::size_t)>{[requestRef, offset = std::size_t{0}](char* buffer, std::size_t bufferSize) mutable -> std::size_t {
                          id body = msg_send<id>(static_cast<id>(requestRef.get()), "HTTPBody"_sel);
                          if (!body)
                              return 0;
                          const NSUInteger totalLength = msg_send<NSUInteger>(body, "length"_sel);
                          if (offset >= static_cast<std::size_t>(totalLength))
                              return 0;
                          const std::size_t available = static_cast<std::size_t>(totalLength) - offset;
                          const std::size_t toCopy = available < bufferSize ? available : bufferSize;
                          struct NuiNSRange
                          {
                              NSUInteger location;
                              NSUInteger length;
                          };
                          const NuiNSRange range = {static_cast<NSUInteger>(offset), static_cast<NSUInteger>(toCopy)};
                          msg_send<void>(body, "getBytes:range:"_sel, buffer, range);
                          offset += toCopy;
                          return toCopy;
                      }}
                    : std::function<std::size_t(char*, std::size_t)>{},
                .headers =
                    [request]() {
                        std::unordered_multimap<std::string, std::string> headerMap;

                        id headers = msg_send<id>(request, "allHTTPHeaderFields"_sel);
                        id keys = msg_send<id>(headers, "allKeys"_sel);
                        for (NSUInteger i = 0; i < msg_send<NSUInteger>(keys, "count"_sel); i++)
                        {
                            id key = msg_send<id>(keys, "objectAtIndex:"_sel, i);
                            id value = msg_send<id>(headers, "objectForKey:"_sel, key);
                            std::string keyStr = msg_send<const char*>(key, "UTF8String"_sel);
                            std::string valueStr = msg_send<const char*>(value, "UTF8String"_sel);

                            headerMap.emplace(keyStr, valueStr);
                        }
                        return headerMap;
                    }(),
                .uri =
                    [url]() {
                        id absoluteString = msg_send<id>(url, "absoluteString"_sel);
                        std::string absoluteStringStr = msg_send<const char*>(absoluteString, "UTF8String"_sel);
                        return absoluteStringStr;
                    }(),
                .method = methodStr,
            };

            auto response = handler->scheme().onRequest(schemeRequest);

            // For bodyFile, auto-populate Content-Length from file size when not already provided.
            // If the file cannot be opened, surface the failure as 500 instead of silently emitting
            // an empty 200 — silent success-with-empty-body is a cache-poisoning footgun.
            std::ifstream bodyFileStream;
            std::optional<std::string> bodyFileErrorBody;
            if (response.bodyFile)
            {
                std::error_code ec;
                const auto size = std::filesystem::file_size(*response.bodyFile, ec);
                if (!ec && response.headers.find("Content-Length") == response.headers.end())
                    response.headers.emplace("Content-Length", std::to_string(size));
                bodyFileStream.open(*response.bodyFile, std::ios::binary);
                if (!bodyFileStream.is_open())
                {
                    bodyFileErrorBody = "Internal Server Error: bodyFile could not be opened.";
                    response.statusCode = 500;
                    response.headers.erase("Content-Length");
                    response.headers.emplace("Content-Length", std::to_string(bodyFileErrorBody->size()));
                    response.headers.erase("Content-Type");
                    response.headers.emplace("Content-Type", "text/plain; charset=utf-8");
                }
            }

            auto headers = msg_send<id>("NSMutableDictionary"_cls, "dictionary"_sel);
            if (response.headers.find("Access-Control-Allow-Origin") == response.headers.end() &&
                !handler->scheme().allowedOrigins.empty())
            {
                auto const& front = handler->scheme().allowedOrigins.front();
                id nsValue = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, front.c_str());
                msg_send<void>(headers, "setObject:forKey:"_sel, nsValue, "Access-Control-Allow-Origin"_str);
            }

            for (const auto& [key, value] : response.headers)
            {
                id nsKey = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, key.c_str());
                id nsValue = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, value.c_str());
                msg_send<void>(headers, "setObject:forKey:"_sel, nsValue, nsKey);
            }

            id nsResponse = msg_send<id>("NSHTTPURLResponse"_cls, "alloc"_sel);
            nsResponse = msg_send<id>(
                nsResponse,
                "initWithURL:statusCode:HTTPVersion:headerFields:"_sel,
                url,
                static_cast<NSInteger>(response.statusCode),
                "HTTP/1.1"_str,
                headers);

            msg_send<void>(task, "didReceiveResponse:"_sel, nsResponse);

            // Deliver body in priority order: bodyFile > bodyReader > body.
            // WKURLSchemeTask supports multiple didReceiveData: calls, so we stream in chunks.
            // Heap-allocate the chunk so we don't put 64 KiB on the (potentially small) dispatch-queue stack.
            constexpr std::size_t chunkSize = 64 * 1024;
            auto chunk = std::make_unique<char[]>(chunkSize);

            auto sendChunk = [&](const void* bytes, std::size_t n) {
                id data = msg_send<id>(
                    "NSData"_cls, "dataWithBytes:length:"_sel, bytes, static_cast<NSUInteger>(n));
                msg_send<void>(task, "didReceiveData:"_sel, data);
            };

            if (bodyFileErrorBody)
            {
                sendChunk(bodyFileErrorBody->data(), bodyFileErrorBody->size());
            }
            else if (response.bodyFile)
            {
                auto readNext = [&]() -> std::size_t {
                    bodyFileStream.read(chunk.get(), chunkSize);
                    return static_cast<std::size_t>(bodyFileStream.gcount());
                };
                for (auto n = readNext(); n > 0; n = readNext())
                    sendChunk(chunk.get(), n);
            }
            else if (response.bodyReader)
            {
                for (auto n = response.bodyReader(chunk.get(), chunkSize); n > 0;
                     n = response.bodyReader(chunk.get(), chunkSize))
                    sendChunk(chunk.get(), n);
            }
            else
            {
                sendChunk(response.body.data(), response.body.size());
            }
            msg_send<void>(task, "didFinish"_sel);
        }

        static void stopURLSchemeTask(id /*self*/, SEL /*_cmd*/, id /*webView*/, id /*task*/)
        {}

        void assignScheme(CustomScheme const& scheme)
        {
            m_scheme = scheme;
        }

      private:
        [[maybe_unused]] Class isa;
        CustomScheme m_scheme;
    };

    static void
    wkWebViewCopyOptionsToConfig(id config, HostNameMappingInfo const* mappingInfo, WindowOptions const& options)
    {
        auto const* opts = &options;
        std::optional<WindowOptions> optCopy;
        if (options.folderMappingScheme)
        {
            optCopy = options;
            optCopy->customSchemes.push_back(
                CustomScheme{
                    .scheme = *options.folderMappingScheme,
                    .allowedOrigins = {"*"},
                    .onRequest =
                        [mappingInfo](CustomSchemeRequest const& req) {
                            return folderMappingResponseFromRequest(req, *mappingInfo);
                        },
                });
            opts = &*optCopy;
        }

        NuiSchemeHandler::registerClass();
        for (auto const& scheme : opts->customSchemes)
        {
            id handler = msg_send<id>("NuiSchemeHandler"_cls, "new"_sel, &scheme);

            auto* nsScheme = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, scheme.scheme.c_str());
            msg_send<void>(config, "setURLSchemeHandler:forURLScheme:"_sel, handler, nsScheme);
        }
    }
}