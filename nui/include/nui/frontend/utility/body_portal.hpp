#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/dom/element.hpp>

#include <memory>

namespace Nui
{
    /**
     * @brief RAII wrapper that renders an ElementRenderer into a detached DOM
     * element and appends it directly to document.body, bypassing any CSS
     * transform / filter / perspective containment on ancestor elements in the
     * Nui component tree.  Useful for overlays (menus, tooltips, modals) that
     * rely on viewport-relative positioning (position:fixed).
     *
     * The element is removed from the DOM when the BodyPortal is destroyed.
     * BodyPortal is non-copyable but move-constructible.
     */
    class BodyPortal
    {
      public:
        explicit BodyPortal(Nui::ElementRenderer renderer)
            : element_{Dom::Element::makeElement(HtmlElement{"div", &RegularHtmlElementBridge})}
        {
            element_->replaceElement(std::move(renderer));
            Nui::val::global("document")["body"].call<void>("appendChild", element_->val());
        }

        BodyPortal(BodyPortal const&) = delete;
        BodyPortal& operator=(BodyPortal const&) = delete;
        BodyPortal(BodyPortal&&) = default;
        BodyPortal& operator=(BodyPortal&&) = default;

        ~BodyPortal()
        {
            if (element_)
                element_->val().call<void>("remove");
        }

      private:
        std::shared_ptr<Dom::Element> element_;
    };
}
