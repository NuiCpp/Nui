#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/dom/element.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>

#include <memory>
#include <optional>
#include <string>

namespace Nui
{
    /**
     * @brief A delocalized element can switch positions in several slots.
     */
    template <typename SlotId>
    class Delocalized
    {
      public:
        Delocalized(Nui::ElementRenderer renderer = {})
            : delocalizedElement_{[&renderer]() -> decltype(delocalizedElement_) {
                if (renderer)
                {
                    auto element = Dom::Element::makeElement(HtmlElement{"div", &RegularHtmlElementBridge});
                    element->replaceElement(renderer);
                    return element;
                }
                else
                {
                    return {};
                }
            }()}
            , slotId_{std::make_unique<Nui::Observed<SlotId>>()}
        {}
        Delocalized(Delocalized const&) = delete;
        Delocalized(Delocalized&&) = default;
        Delocalized& operator=(Delocalized const&) = delete;
        Delocalized& operator=(Delocalized&&) = default;

        std::shared_ptr<Dom::Element> element()
        {
            return delocalizedElement_;
        }
        void element(Nui::ElementRenderer renderer)
        {
            delocalizedElement_ = Dom::Element::makeElement(HtmlElement{"div", &RegularHtmlElementBridge});
            delocalizedElement_->replaceElement(renderer);
        }
        bool hasElement() const
        {
            return delocalizedElement_ != nullptr;
        }
        void initializeIfEmpty(Nui::ElementRenderer renderer)
        {
            if (!hasElement())
                element(std::move(renderer));
        }

        void slot(SlotId slot)
        {
            *slotId_ = slot;
        }
        SlotId slot() const
        {
            return slotId_->value();
        }

        template <typename SlotIdB>
        friend ElementRenderer delocalizedSlot(
            SlotIdB slot,
            Delocalized<SlotIdB>& delocalizedElement,
            std::vector<Attribute> wrapperAttributes,
            Nui::ElementRenderer alternative);

      private:
        std::shared_ptr<Dom::Element> delocalizedElement_;
        // Wrapped in a unique_ptr for pointer stability.
        std::unique_ptr<Nui::Observed<SlotId>> slotId_;
    };

    template <typename SlotId>
    ElementRenderer delocalizedSlot(
        SlotId slot,
        Delocalized<SlotId>& delocalizedElement,
        std::vector<Attribute> wrapperAttributes = {},
        ElementRenderer alternative = Elements::div{Attributes::style = "display: none"}())
    {
        using namespace Elements;
        using namespace Attributes;

        auto element = delocalizedElement.delocalizedElement_;

        return Elements::div{std::move(wrapperAttributes)}(
            observe(*delocalizedElement.slotId_),
            [element, slot, &observedId = *delocalizedElement.slotId_, alternative]() mutable {
                return [element, slot, &observedId, alternative](
                           Dom::Element& wrapper, Renderer const& gen) -> std::shared_ptr<Dom::Element> {
                    if (element && slot == observedId.value())
                    {
                        wrapper.val().call<void>("appendChild", element->val());
                        return nil()(wrapper, gen);
                    }
                    else
                    {
                        if (alternative)
                            return alternative(wrapper, gen);
                        else
                            return nil()(wrapper, gen);
                    }
                };
            });
    }

    inline ElementRenderer delocalizedSlot(
        char const* slot,
        Delocalized<std::string>& delocalizedElement,
        std::vector<Attribute> wrapperAttributes = {},
        ElementRenderer alternative = Elements::div{Attributes::style = "display: none"}())
    {
        return delocalizedSlot<std::string>(
            std::string{slot}, delocalizedElement, std::move(wrapperAttributes), std::move(alternative));
    }
}