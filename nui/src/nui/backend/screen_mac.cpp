#include <nui/screen.hpp>
#include <nui/utility/scope_exit.hpp>

#include <CoreGraphics/CGDisplayConfiguration.h>
#include <IOKit/graphics/IOGraphicsLib.h>

#include <iostream>

namespace Nui
{
    namespace
    {
        static void KeyArrayCallback(const void* key, const void*, void* context)
        {
            CFArrayAppendValue(reinterpret_cast<CFMutableArrayRef>(context), key);
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        std::string getDisplayName(CGDirectDisplayID displayID)
        {
            CFStringRef displayName = NULL;
            std::string strResult;
            io_service_t service = CGDisplayIOServicePort(displayID);
            NUI_ON_SCOPE_EXIT
            {
                if (service)
                    IOObjectRelease(service);
            };
            CFDictionaryRef info = IODisplayCreateInfoDictionary(service, kIODisplayOnlyPreferredName);
            NUI_ON_SCOPE_EXIT
            {
                if (info)
                    CFRelease(info);
            };

            CFDictionaryRef names =
                reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(info, CFSTR(kDisplayProductName)));

            if (names && CFDictionaryGetCount(names))
            {
                CFMutableArrayRef langKeys = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
                CFDictionaryApplyFunction(
                    names,
                    reinterpret_cast<CFDictionaryApplierFunction>(KeyArrayCallback),
                    reinterpret_cast<void*>(langKeys));
                CFArrayRef orderLangKeys = CFBundleCopyPreferredLocalizationsFromArray(langKeys);

                NUI_ON_SCOPE_EXIT
                {
                    if (orderLangKeys)
                        CFRelease(orderLangKeys);
                    if (langKeys)
                        CFRelease(langKeys);
                };

                if (orderLangKeys && CFArrayGetCount(orderLangKeys))
                {
                    CFStringRef langKey = reinterpret_cast<CFStringRef>(CFArrayGetValueAtIndex(orderLangKeys, 0));
                    displayName = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(names, langKey));

                    NUI_ON_SCOPE_EXIT
                    {
                        if (displayName)
                            CFRelease(displayName);
                    };

                    if (displayName)
                    {
                        char buffer[4096] = {'\0'};
                        if (CFStringGetCString(displayName, buffer, sizeof(buffer), kCFStringEncodingUTF8))
                            strResult = buffer;
                    }
                }
            }
            return strResult;
        }
#pragma clang diagnostic pop
    }

    std::vector<Display> Screen::getDisplays()
    {
        std::vector<Display> displays;
        CGDirectDisplayID displayIDs[32];
        uint32_t count;
        CGGetActiveDisplayList(32, displayIDs, &count);
        for (uint32_t i = 0; i < count; ++i)
        {
            const auto rect = CGDisplayBounds(displayIDs[i]);
            displays.emplace_back(
                static_cast<int32_t>(rect.origin.x),
                static_cast<int32_t>(rect.origin.y),
                static_cast<int32_t>(rect.size.width),
                static_cast<int32_t>(rect.size.height),
                displayIDs[i] == CGMainDisplayID(),
                getDisplayName(displayIDs[i]));
        }
        return displays;
    }

    Display Screen::getPrimaryDisplay()
    {
        const auto rect = CGDisplayBounds(CGMainDisplayID());
        return Display(
            static_cast<int32_t>(rect.origin.x),
            static_cast<int32_t>(rect.origin.y),
            static_cast<int32_t>(rect.size.width),
            static_cast<int32_t>(rect.size.height),
            true,
            getDisplayName(CGMainDisplayID()));
    }
}