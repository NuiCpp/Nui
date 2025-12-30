#pragma once

#include <nui/frontend/api/ui_event.hpp>

namespace Nui::WebApi
{
    /**
     * @brief A mouse event class.
     *
     * @see https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent
     */
    class MouseEvent : public UiEvent
    {
      public:
        explicit MouseEvent(Nui::val event);

        /**
         * @brief Returns a boolean value that is true if the Alt (Option or ⌥ on macOS) key was active when the key
         * event was generated.
         */
        bool altKey() const;

        /**
         * @brief The button number that was pressed or released (if applicable) when the mouse event was fired.
         *
         * @return int
         */
        int button() const;

        /**
         * @brief The buttons being pressed (if any) when the mouse event was fired.
         *
         * @return int
         */
        int buttons() const;

        /**
         * @brief The X coordinate of the mouse pointer in viewport coordinates.
         * @see https://developer.mozilla.org/en-US/docs/Web/API/CSSOM_view_API/Coordinate_systems#viewport
         */
        double clientX() const;

        /**
         * @brief The Y coordinate of the mouse pointer in viewport coordinates.
         * @see https://developer.mozilla.org/en-US/docs/Web/API/CSSOM_view_API/Coordinate_systems#viewport
         */
        double clientY() const;

        /**
         * @brief The X coordinate of the mouse pointer in viewport coordinates.
         * @see https://developer.mozilla.org/en-US/docs/Web/API/CSSOM_view_API/Coordinate_systems#viewport
         */
        double x() const;

        /**
         * @brief The Y coordinate of the mouse pointer in viewport coordinates.
         * @see https://developer.mozilla.org/en-US/docs/Web/API/CSSOM_view_API/Coordinate_systems#viewport
         */
        double y() const;

        /**
         * @brief The X coordinate of the mouse pointer relative to the position of the last mousemove event.
         */
        double movementX() const;

        /**
         * @brief The Y coordinate of the mouse pointer relative to the position of the last mousemove event.
         */
        double movementY() const;

        /**
         * @brief The X coordinate of the mouse pointer relative to the position of the padding edge of the target
         * node.
         */
        double offsetX() const;

        /**
         * @brief The Y coordinate of the mouse pointer relative to the position of the padding edge of the target node.
         */
        double offsetY() const;

        /**
         * @brief The X coordinate of the mouse pointer relative to the whole document.
         */
        double pageX() const;

        /**
         * @brief The Y coordinate of the mouse pointer relative to the whole document.
         */
        double pageY() const;

        /**
         * @brief The X coordinate of the mouse pointer in screen coordinates.
         * @see https://developer.mozilla.org/en-US/docs/Web/API/CSSOM_view_API/Coordinate_systems#screen
         */
        double screenX() const;

        /**
         * @brief The Y coordinate of the mouse pointer in screen coordinates.
         * @see https://developer.mozilla.org/en-US/docs/Web/API/CSSOM_view_API/Coordinate_systems#screen
         */
        double screenY() const;

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
         * @brief The secondary target for the event, if there is one.
         *
         * @return std::optional<Nui::val> nullopt, or a valid EventTarget object.
         */
        std::optional<Nui::val> relatedTarget() const;
    };
}