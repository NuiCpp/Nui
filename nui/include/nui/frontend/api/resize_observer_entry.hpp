#pragma once

#include <nui/frontend/api/dom_rect_readonly.hpp>
#include <nui/frontend/val_wrapper.hpp>

namespace Nui::WebApi
{
    /**
     * @brief A ResizeObserverEntry interface represents the size of an observed element after a resize.
     *
     * @see https://developer.mozilla.org/en-US/docs/Web/API/ResizeObserverEntry
     */
    class ResizeObserverEntry : public ValWrapper
    {
      public:
        explicit ResizeObserverEntry(Nui::val event);

        struct BoxSize
        {
            double blockSize{0.};
            double inlineSize{0.};
        };

        /**
         * @brief The borderBoxSize read-only property of the ResizeObserverEntry interface returns an array containing
         * the new border box size of the observed element when the callback is run.
         */
        std::vector<BoxSize> borderBoxSize() const;

        /**
         * @brief The contentBoxSize read-only property of the ResizeObserverEntry interface returns an array containing
         * the new content box size of the observed element when the callback is run.
         */
        std::vector<BoxSize> contentBoxSize() const;

        /**
         * @brief The contentRect read-only property of the ResizeObserverEntry interface returns a DOMRectReadOnly
         * object containing the new size of the observed element when the callback is run. Note that this is better
         * supported than ResizeObserverEntry.borderBoxSize or ResizeObserverEntry.contentBoxSize, but it is left over
         * from an earlier implementation of the Resize Observer API, is still included in the spec for web compat
         * reasons, and may be deprecated in future versions.
         */
        DomRectReadOnly contentRect() const;

        /**
         * @brief The devicePixelContentBoxSize read-only property of the ResizeObserverEntry interface returns an array
         * containing the size in device pixels of the observed element when the callback is run.
         */
        std::vector<BoxSize> devicePixelContentBoxSize() const;

        /**
         * @brief The target read-only property of the ResizeObserverEntry interface returns a reference to the Element
         * or SVGElement that is being observed.
         */
        Nui::val target() const;
    };
}