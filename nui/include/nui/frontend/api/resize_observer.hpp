#pragma once

#include <nui/frontend/api/resize_observer_entry.hpp>
#include <nui/frontend/val_wrapper.hpp>
#include <nui/utility/move_detector.hpp>

#include <functional>

namespace Nui::WebApi
{
    /**
     * @brief A resize observer class, used for observing resize events on elements.
     *
     * @see https://developer.mozilla.org/en-US/docs/Web/API/ResizeObserver/ResizeObserver
     */
    class ResizeObserver : public ValWrapper
    {
      public:
        explicit ResizeObserver(
            std::function<void(std::vector<ResizeObserverEntry> const&, ResizeObserver const&)> callback);
        ~ResizeObserver() override;
        ResizeObserver(ResizeObserver const&) = delete;
        ResizeObserver(ResizeObserver&&) noexcept = default;
        ResizeObserver& operator=(ResizeObserver const&) = delete;
        ResizeObserver& operator=(ResizeObserver&&) noexcept = default;

        explicit ResizeObserver(Nui::val event);

        /**
         * @brief The disconnect() method of the ResizeObserver interface unobserves all observed Element or SVGElement
         * targets.
         */
        void disconnect() const;

        /**
         * @brief The observe() method of the ResizeObserver interface starts observing the specified Element or
         * SVGElement.
         */
        void observe(Nui::val target) const;

        struct ObserveOptions
        {
            std::string box;
        };
        /**
         * @brief The observe() method of the ResizeObserver interface starts observing the specified Element or
         * SVGElement.
         *
         * @param options An options object allowing you to set options for the observation. Currently this only has one
         * possible option that can be set:
         *   box
         *       Sets which box model the observer will observe changes to. Possible values are:
         *       content-box (the default)
         *           Size of the content area as defined in CSS.
         *       border-box
         *           Size of the box border area as defined in CSS.
         *       device-pixel-content-box
         *           The size of the content area as defined in CSS, in device pixels, before applying any CSS
         *           transforms on the element or its ancestors.
         */
        void observe(Nui::val target, ObserveOptions const& options) const;

        /**
         * @brief The unobserve() method of the ResizeObserver interface ends the observing of a specified Element or
         * SVGElement.
         *
         * @param target
         */
        void unobserve(Nui::val target) const;

      private:
        Nui::MoveDetector moveDetector_;
        std::function<void(std::vector<ResizeObserverEntry> const&, ResizeObserver const&)> callback_;
    };
}