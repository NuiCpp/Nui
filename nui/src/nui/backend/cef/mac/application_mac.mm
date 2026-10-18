#include "application_mac.hpp"
#include "../engine_state.hpp"

#include <include/cef_application_mac.h>
#include <include/wrapper/cef_library_loader.h>

#include "../../mac_main_menu.ipp"

#import <AppKit/AppKit.h>
#include <objc/runtime.h>

#include <stdexcept>

namespace
{
    BOOL handlingSendEvent = NO;
}

@interface NuiCefApplication : NSApplication <CefAppProtocol>
@end

@implementation NuiCefApplication
- (BOOL)isHandlingSendEvent
{
    return handlingSendEvent;
}

- (void)setHandlingSendEvent:(BOOL)value
{
    handlingSendEvent = value;
}

- (void)sendEvent:(NSEvent*)event
{
    CefScopedSendingEvent sendingEvent;
    [super sendEvent:event];
}

// Dock Quit, logout and Cmd+Q. The default calls exit() inside the run loop, Chromium needs the windows to close and
// the loop to end instead.
- (void)terminate:(id)sender
{
    Nui::Detail::Cef::EngineState::instance().closeAllWindows();
}
@end

@interface NuiCefApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@implementation NuiCefApplicationDelegate
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)application
{
    return YES;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)application
{
    return NO;
}
@end

namespace
{
    BOOL nuiIsHandlingSendEvent(id, SEL)
    {
        return handlingSendEvent;
    }

    void nuiSetHandlingSendEvent(id, SEL, BOOL value)
    {
        handlingSendEvent = value;
    }

    IMP originalSendEvent = nullptr;

    void nuiSendEvent(id self, SEL selector, NSEvent* event)
    {
        CefScopedSendingEvent sendingEvent;
        reinterpret_cast<void (*)(id, SEL, NSEvent*)>(originalSendEvent)(self, selector, event);
    }

    void nuiTerminate(id, SEL, id)
    {
        Nui::Detail::Cef::EngineState::instance().closeAllWindows();
    }

    // Another library created NSApp first: give its class what CEF expects from the application.
    void adoptExistingApplication(NSApplication* application)
    {
        Class applicationClass = [application class];
        if ([application conformsToProtocol:@protocol(CefAppProtocol)])
            return;
        class_addProtocol(applicationClass, @protocol(CrAppProtocol));
        class_addProtocol(applicationClass, @protocol(CrAppControlProtocol));
        class_addProtocol(applicationClass, @protocol(CefAppProtocol));
        class_addMethod(applicationClass, @selector(isHandlingSendEvent), reinterpret_cast<IMP>(&nuiIsHandlingSendEvent), "c@:");
        class_addMethod(
            applicationClass, @selector(setHandlingSendEvent:), reinterpret_cast<IMP>(&nuiSetHandlingSendEvent), "v@:c");
        Method sendEvent = class_getInstanceMethod(applicationClass, @selector(sendEvent:));
        originalSendEvent = method_setImplementation(sendEvent, reinterpret_cast<IMP>(&nuiSendEvent));
        Method terminate = class_getInstanceMethod(applicationClass, @selector(terminate:));
        method_setImplementation(terminate, reinterpret_cast<IMP>(&nuiTerminate));
    }
}

namespace Nui::Detail::Cef
{
    void prepareMacApplication()
    {
        // Loaded once and never unloaded, CEF objects may outlive any scope.
        static auto* libraryLoader = new CefScopedLibraryLoader();
        static bool loaded = false;
        if (!loaded)
        {
            if (!libraryLoader->LoadInMain())
            {
                throw std::runtime_error(
                    "Nui: the CEF framework was not found. With the CEF engine the executable must run inside its app "
                    "bundle, see nui_configure_application in CMake");
            }
            loaded = true;
        }

        @autoreleasepool
        {
            if (NSApp == nil)
                [NuiCefApplication sharedApplication];
            else
                adoptExistingApplication(NSApp);
            static NuiCefApplicationDelegate* delegate = [[NuiCefApplicationDelegate alloc] init];
            if ([NSApp delegate] == nil)
                [NSApp setDelegate:delegate];
            Nui::MacOs::installDefaultMainMenu("terminate:");
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::uintptr_t windowOfView(cef_window_handle_t handle)
    {
        auto* view = static_cast<NSView*>(handle);
        return reinterpret_cast<std::uintptr_t>([view window]);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void withAutoreleasePool(std::function<void()> const& function)
    {
        @autoreleasepool
        {
            function();
        }
    }
}
