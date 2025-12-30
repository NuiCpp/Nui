#pragma once

#include <nui/frontend/val_wrapper.hpp>

#include <chrono>
#include <string>

namespace Nui::WebApi
{
    enum class EventPhase
    {
        NONE = 0,
        CAPTURING_PHASE = 1,
        AT_TARGET = 2,
        BUBBLING_PHASE = 3
    };

    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
     */
    class Event : public ValWrapper
    {
      public:
        explicit Event(Nui::val event);

        /**
         * @brief A boolean value indicating whether or not the event bubbles up through the DOM.
         */
        bool bubbles() const;

        /**
         * @brief A boolean value indicating whether the event is cancelable.
         */
        bool cancelable() const;

        /**
         * @brief A boolean indicating whether or not the event can bubble across the boundary between the shadow DOM
         * and the regular DOM.
         */
        bool composed() const;

        /**
         * @brief A reference to the currently registered target for the event. This is the object to which the event is
         * currently slated to be sent. It's possible this has been changed along the way through retargeting.
         */
        Nui::val currentTarget() const;

        /**
         * @brief Indicates whether or not the call to event.preventDefault() canceled the event.
         *
         * @return true
         * @return false
         */
        bool defaultPrevented() const;

        /**
         * @brief Indicates which phase of the event flow is being processed.
         *
         * @return EventPhase
         */
        EventPhase eventPhase() const;

        /**
         * @brief Indicates whether or not the event was initiated by the browser (after a user click, for instance) or
         * by a script (using an event creation method, for example).
         */
        bool isTrusted() const;

        /**
         * @brief A reference to the object to which the event was originally dispatched.
         *
         * @return Nui::val
         */
        Nui::val target() const;

        /**
         * @brief The time at which the event was created (in milliseconds). By specification, this value is time since
         * epoch—but in reality, browsers' definitions vary. In addition, work is underway to change this to be a
         * DOMHighResTimeStamp instead.
         */
        std::chrono::milliseconds timeStamp() const;

        /**
         * @brief The name identifying the type of the event, e.g., "click", "hashchange", or "submit".
         */
        std::string type() const;

        /**
         * @brief The composedPath() method of the Event interface returns the event's path which is an array of the
         * objects on which listeners will be invoked. This does not include nodes in shadow trees if the shadow root
         * was created with its ShadowRoot.mode closed.
         */
        std::vector<Nui::val> composedPath() const;

        /**
         * @brief Cancels the event (if it is cancelable).
         */
        void preventDefault() const;

        /**
         * @brief For this particular event, prevent all other listeners from being called. This includes listeners
         * attached to the same element as well as those attached to elements that will be traversed later (during the
         * capture phase, for instance).
         */
        void stopImmediatePropagation() const;

        /**
         * @brief Stops the propagation of events further along in the DOM.
         */
        void stopPropagation() const;
    };
}