#include <nui/frontend/utility/stabilize.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>

namespace Nui
{
    void StableElement::reset()
    {
        reset_ = true;
    }

    void StableElement::destroy()
    {
        stableElement_.reset();
    }

    Dom::Element& StableElement::stableElement()
    {
        return *stableElement_;
    }

    Dom::Element const& StableElement::stableElement() const
    {
        return *stableElement_;
    }

    ElementRenderer stabilize(StableElement& stableElement, ElementRenderer const& encapsulatedRenderer)
    {
        return [encapsulatedRenderer,
                &stableElement](Dom::Element& actualParent, Renderer const& gen) -> std::shared_ptr<Dom::Element> {
            if (stableElement.reset_ || !stableElement.stableElement_)
            {
                stableElement.reset_ = false;
                // Needs to be valid element for replace and fragments:
                stableElement.stableElement_ = Dom::Element::makeElement(HtmlElement{"div", &RegularHtmlElementBridge});
                stableElement.stableElement_->replaceElement(encapsulatedRenderer);
                return HtmlElement{"stablerror_slot", &RegularHtmlElementBridge}()(actualParent, gen)
                    ->slotFor(stableElement.stableElement_);
            }
            return nil()(actualParent, gen);
        };
    }
}