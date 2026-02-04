#include "mac_helpers/helpers.hpp"
#include "mac_helpers/type_encodings.hpp"
#include "mac_helpers/class.hpp"

namespace Nui::MacOs
{
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
            NuiSchemeHandler* handler = reinterpret_cast<NuiSchemeHandler*>(self);

            id request = msg_send<id>(task, "request"_sel);
            id method = msg_send<id>(request, "HTTPMethod"_sel);
            std::string methodStr = msg_send<const char*>(method, "UTF8String"_sel);
            id url = msg_send<id>(request, "URL"_sel);

            CustomSchemeRequest schemeRequest = {
                .scheme = handler->scheme().scheme,
                .getContent = std::function<std::string()>{[request]() {
                    id body = msg_send<id>(request, "HTTPBody"_sel);
                    if (!body)
                        return std::string{};
                    std::string bodyStr = msg_send<const char*>(body, "UTF8String"_sel);
                    return bodyStr;
                }},
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

            const auto response = handler->scheme().onRequest(schemeRequest);

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
                response.statusCode,
                "HTTP/1.1"_str,
                headers);

            msg_send<void>(task, "didReceiveResponse:"_sel, nsResponse);

            auto data = msg_send<id>(
                "NSData"_cls,
                "dataWithBytes:length:"_sel,
                response.body.data(),
                static_cast<NSUInteger>(response.body.size()));
            msg_send<void>(task, "didReceiveData:"_sel, data);
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

    id wkWebViewConfigurationFromOptions(HostNameMappingInfo const* mappingInfo, WindowOptions const& options)
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

        auto config = msg_send<id>("WKWebViewConfiguration"_cls, "new"_sel);
        NuiSchemeHandler::registerClass();
        for (auto const& scheme : opts->customSchemes)
        {
            id handler = msg_send<id>("NuiSchemeHandler"_cls, "new"_sel, &scheme);

            auto* nsScheme = msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, scheme.scheme.c_str());
            msg_send<void>(config, "setURLSchemeHandler:forURLScheme:"_sel, handler, nsScheme);
        }
        return config;
    }
}