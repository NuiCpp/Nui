#pragma once

#include <nui/frontend/api/event.hpp>

namespace Nui::WebApi
{
    /**
     * @brief A keyboard event class.
     *
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Element/keyup_event
     */
    class KeyboardEvent : public Event
    {
      public:
        explicit KeyboardEvent(Nui::val event);

        /**
         * @brief Returns a boolean value that is true if the Alt (Option or ⌥ on macOS) key was active when the key
         * event was generated.
         */
        bool altKey() const;

        /**
         * @brief Returns a boolean value that is true if the Ctrl key was active when the key event was generated.
         */
        bool ctrlKey() const;

        /**
         * @brief Returns a boolean value that is true if the Shift key was active when the key event was generated.
         */
        bool shiftKey() const;

        /**
         * @brief Returns a boolean value that is true if the Meta key (on Mac keyboards, the ⌘ Command key; on Windows
         * keyboards, the Windows key (⊞)) was active when the key event was generated.
         */
        bool metaKey() const;

        /**
         * @brief Returns a string with the code value of the physical key represented by the event.
         *
         * Warning: This ignores the user's keyboard layout, so that if the user presses the key at the "Y"
         * position in a QWERTY keyboard layout (near the middle of the row above the home row), this will always return
         * "KeyY", even if the user has a QWERTZ keyboard (which would mean the user expects a "Z" and all the other
         * properties would indicate a "Z") or a Dvorak keyboard layout (where the user would expect an "F"). If you
         * want to display the correct keystrokes to the user, you can use Keyboard.getLayoutMap().
         *
         * @return std::string
         */
        std::string code() const;

        /**
         * @brief Returns a boolean value that is true if the event is fired between after compositionstart and before
         * compositionend.
         */
        bool isComposing() const;

        /**
         * @brief Returns a string representing the key value of the key represented by the event.
         */
        std::string key() const;

        /**
         * @brief Returns a number representing the location of the key on the keyboard or other input device. A list of
         * the constants identifying the locations is shown in Keyboard locations.
         */
        int location() const;

        /**
         * @brief Returns a boolean value that is true if the key is being held down such that it is automatically
         * repeating.
         */
        bool repeat() const;
    };
}