#include "mac_helpers/helpers.hpp"

namespace Nui::MacOs
{
    namespace
    {
        constexpr NSUInteger modifierShift = 1u << 17u;
        constexpr NSUInteger modifierControl = 1u << 18u;
        constexpr NSUInteger modifierOption = 1u << 19u;
        constexpr NSUInteger modifierCommand = 1u << 20u;

        id makeString(std::string const& text)
        {
            return msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, text.c_str());
        }

        void addItem(id menu, std::string const& title, char const* action, char const* key, NSUInteger modifiers)
        {
            id item = msg_send<id>("NSMenuItem"_cls, "alloc"_sel);
            item = msg_send<id>(
                item,
                "initWithTitle:action:keyEquivalent:"_sel,
                makeString(title),
                sel_registerName(action),
                makeString(key));
            msg_send<void>(item, "setKeyEquivalentModifierMask:"_sel, modifiers);
            msg_send<void>(menu, "addItem:"_sel, item);
            msg_send<void>(item, "release"_sel);
        }

        void addSeparator(id menu)
        {
            msg_send<void>(menu, "addItem:"_sel, msg_send<id>("NSMenuItem"_cls, "separatorItem"_sel));
        }

        id addSubmenu(id mainMenu, std::string const& title)
        {
            id menu = msg_send<id>(msg_send<id>("NSMenu"_cls, "alloc"_sel), "initWithTitle:"_sel, makeString(title));
            id item = msg_send<id>(msg_send<id>("NSMenuItem"_cls, "alloc"_sel), "init"_sel);
            msg_send<void>(item, "setSubmenu:"_sel, menu);
            msg_send<void>(mainMenu, "addItem:"_sel, item);
            msg_send<void>(item, "release"_sel);
            msg_send<void>(menu, "release"_sel);
            return menu;
        }
    }

    /**
     * @brief Installs the standard application, edit and window menus.
     * @details WKWebView only receives Cmd+C/V/X/A/Z through the responder chain of a menu that
     *          carries those key equivalents. Quit closes the window instead of terminating, so the
     *          application unwinds through its normal shutdown path.
     * @param quitAction Selector of the Quit item.
     */
    void installDefaultMainMenu(char const* quitAction = "performClose:")
    {
        id pool = msg_send<id>(msg_send<id>("NSAutoreleasePool"_cls, "alloc"_sel), "init"_sel);

        id application = msg_send<id>("NSApplication"_cls, "sharedApplication"_sel);
        id processName = msg_send<id>(msg_send<id>("NSProcessInfo"_cls, "processInfo"_sel), "processName"_sel);
        const std::string name = msg_send<char const*>(processName, "UTF8String"_sel);

        id mainMenu = msg_send<id>(msg_send<id>("NSMenu"_cls, "alloc"_sel), "initWithTitle:"_sel, makeString(""));

        id appMenu = addSubmenu(mainMenu, name);
        addItem(appMenu, "About " + name, "orderFrontStandardAboutPanel:", "", 0);
        addSeparator(appMenu);
        addItem(appMenu, "Hide " + name, "hide:", "h", modifierCommand);
        addItem(appMenu, "Hide Others", "hideOtherApplications:", "h", modifierCommand | modifierOption);
        addItem(appMenu, "Show All", "unhideAllApplications:", "", 0);
        addSeparator(appMenu);
        addItem(appMenu, "Quit " + name, quitAction, "q", modifierCommand);

        id editMenu = addSubmenu(mainMenu, "Edit");
        addItem(editMenu, "Undo", "undo:", "z", modifierCommand);
        addItem(editMenu, "Redo", "redo:", "z", modifierCommand | modifierShift);
        addSeparator(editMenu);
        addItem(editMenu, "Cut", "cut:", "x", modifierCommand);
        addItem(editMenu, "Copy", "copy:", "c", modifierCommand);
        addItem(editMenu, "Paste", "paste:", "v", modifierCommand);
        addItem(editMenu, "Select All", "selectAll:", "a", modifierCommand);

        id windowMenu = addSubmenu(mainMenu, "Window");
        addItem(windowMenu, "Minimize", "performMiniaturize:", "m", modifierCommand);
        addItem(windowMenu, "Zoom", "performZoom:", "", 0);
        addItem(windowMenu, "Enter Full Screen", "toggleFullScreen:", "f", modifierCommand | modifierControl);
        msg_send<void>(application, "setWindowsMenu:"_sel, windowMenu);

        msg_send<void>(application, "setMainMenu:"_sel, mainMenu);
        msg_send<void>(mainMenu, "release"_sel);

        msg_send<void>(pool, "drain"_sel);
    }
}
