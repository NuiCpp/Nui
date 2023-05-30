#include <nui/frontend/utility/stabilize.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>

namespace Nui
{
    void StableElement::reset()
    {
        stableElement_ = {};
    }

    ElementRenderer stabilize(StableElement& stableElement, ElementRenderer const& encapsulatedRenderer)
    {
        return [encapsulatedRenderer,
                &stableElement](Dom::Element& actualParent, Renderer const& gen) -> std::shared_ptr<Dom::Element> {
            if (!stableElement.stableElement_)
            {
                // Needs to be valid element for replace and fragments:
                stableElement.stableElement_ = Dom::Element::makeElement(HtmlElement{"div"});
                stableElement.stableElement_->replaceElement(encapsulatedRenderer);
                return HtmlElement{"stablerror_slot"}()(actualParent, gen)->slotFor(stableElement.stableElement_);
            }
            return nil()(actualParent, gen);
        };
    }
}