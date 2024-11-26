#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/utility/tuple_for_each.hpp>

#include <nui/frontend/val.hpp>

#include <concepts>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Nui::Dom
{
    namespace Detail
    {
        static void destroyByRemove(Nui::val& val)
        {
            val.call<void>("remove");
        }
        [[maybe_unused]] static void destroyByParentChildRemoval(Nui::val& val)
        {
            if (val.hasOwnProperty("parentNode"))
            {
                auto parent = val["parentNode"];
                if (!parent.isUndefined() && !parent.isNull())
                    parent.call<void>("removeChild", val);
                else
                    val.call<void>("remove");
            }
            else
                val.call<void>("remove");
        }
        static void doNotDestroy(Nui::val&)
        {}
    }

    class Element : public ChildlessElement
    {
      public:
        using collection_type = std::vector<std::shared_ptr<Element>>;
        using iterator = collection_type::iterator;
        using const_iterator = collection_type::const_iterator;
        using value_type = collection_type::value_type;

        explicit Element(HtmlElement const& elem)
            : ChildlessElement{elem}
            , children_{}
            , unsetup_{}
            , deferredSetup_{}
        {}

        /**
         * @brief This constructor takes ownership of a val.
         * This val must not be managed by any other element.
         *
         * @param val
         */
        explicit Element(Nui::val val)
            : ChildlessElement{std::move(val)}
            , children_{}
            , unsetup_{}
            , deferredSetup_{}
        {}

        explicit Element()
            : ChildlessElement{}
            , children_{}
            , unsetup_{}
            , deferredSetup_{}
        {}

        Element(Element const&) = delete;
        Element(Element&&) = delete;
        Element& operator=(Element const&) = delete;
        Element& operator=(Element&&) = delete;

        ~Element() override
        {
            clearChildren();
            destroy_(element_);
        }

        static std::shared_ptr<Element> makeElement(HtmlElement const& element)
        {
            auto elem = std::make_shared<Element>(element);
            elem->setup(element);
            return elem;
        }

        iterator begin()
        {
            return std::begin(children_);
        }
        iterator end()
        {
            return std::end(children_);
        }
        const_iterator begin() const
        {
            return std::begin(children_);
        }
        const_iterator end() const
        {
            return std::end(children_);
        }

        void appendElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Append});
        }
        auto appendElement(HtmlElement const& element)
        {
            auto elem = makeElement(element);
            element_.call<Nui::val>("appendChild", elem->element_);
            if (elem->deferredSetup_)
                elem->deferredSetup_(element);
            return children_.emplace_back(std::move(elem));
        }
        auto slotFor(value_type const& value)
        {
            clearChildren();
            if (unsetup_)
                unsetup_();
            unsetup_ = {};

            element_.call<Nui::val>("replaceWith", value->val());
            element_ = value->val();
            destroy_ = Detail::doNotDestroy;
            return shared_from_base<Element>();
        }
        void replaceElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Replace});
        }
        auto replaceElement(HtmlElement const& element)
        {
            replaceElementImpl(element);
            return shared_from_base<Element>();
        }
        auto emplaceElement(HtmlElement const& element)
        {
            if (!element_.isUndefined())
                throw std::runtime_error("Element is not empty, cannot emplace");

            element_ = createElement(element);
            setup(element);
            if (deferredSetup_)
                deferredSetup_(element);
            return shared_from_base<Element>();
        }
        auto emplaceElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Emplace});
            return shared_from_base<Element>();
        }

        void setTextContent(std::string const& text)
        {
            element_.set("textContent", text);
        }
        void setTextContent(char const* text)
        {
            element_.set("textContent", text);
        }
        void setTextContent(std::string_view text)
        {
            element_.set("textContent", text);
        }

        void
        appendElements(std::vector<std::function<std::shared_ptr<Element>(Element&, Renderer const&)>> const& elements)
        {
            for (auto const& element : elements)
                appendElement(element);
        }

        auto insert(iterator where, HtmlElement const& element)
        {
            if (where == end())
                return appendElement(element);
            auto elem = makeElement(element);
            element_.call<Nui::val>("insertBefore", elem->element_, (*where)->element_);
            if (elem->deferredSetup_)
                elem->deferredSetup_(element);
            return *children_.insert(where, std::move(elem));
        }

        /**
         * @brief Relies on weak_from_this and cannot be used from the constructor
         */
        void setup(HtmlElement const& element)
        {
            std::vector<std::function<void()>> eventClearers;
            eventClearers.reserve(element.attributes().size());
            std::vector<std::size_t> deferredIndices;

            for (std::size_t i = 0; i != element.attributes().size(); ++i)
            {
                auto const& attribute = element.attributes()[i];

                if (attribute.defer())
                {
                    deferredIndices.push_back(i);
                    continue;
                }
                if (attribute.isRegular())
                    attribute.setOn(*this);

                auto clear = attribute.getEventClear();
                if (clear)
                {
                    const auto id = attribute.createEvent(weak_from_base<Element>());
                    if (id != EventContext::invalidEventId)
                    {
                        eventClearers.push_back([clear = std::move(clear), id]() {
                            clear(id);
                        });
                    }
                }
            }
            if (!eventClearers.empty())
            {
                eventClearers.shrink_to_fit();
                unsetup_ = [eventClearers = std::move(eventClearers)]() {
                    for (auto const& clear : eventClearers)
                        clear();
                };
            }
            else
            {
                unsetup_ = []() {};
            }

            if (!deferredIndices.empty())
            {
                deferredSetup_ = [this, deferredIndices = std::move(deferredIndices)](HtmlElement const& element) {
                    std::vector<std::function<void()>> eventClearers;
                    eventClearers.reserve(deferredIndices.size());

                    for (auto index : deferredIndices)
                    {
                        auto const& attribute = element.attributes()[index];

                        if (attribute.isRegular())
                            attribute.setOn(*this);

                        auto clear = attribute.getEventClear();
                        if (clear)
                        {
                            const auto id = attribute.createEvent(weak_from_base<Element>());
                            if (id != EventContext::invalidEventId)
                            {
                                eventClearers.push_back([clear = std::move(clear), id]() {
                                    clear(id);
                                });
                            }
                        }
                    }
                    if (!eventClearers.empty())
                    {
                        eventClearers.shrink_to_fit();
                        unsetup_ = [unsetup1 = std::move(unsetup_), eventClearers = std::move(eventClearers)]() {
                            unsetup1();
                            for (auto const& clear : eventClearers)
                                clear();
                        };
                    }
                };
            }
        }

        auto insert(std::size_t where, HtmlElement const& element)
        {
            if (where >= children_.size())
                return appendElement(element);
            return insert(begin() + static_cast<decltype(children_)::difference_type>(where), element);
        }

        auto& operator[](std::size_t index)
        {
            return children_[index];
        }
        auto const& operator[](std::size_t index) const
        {
            return children_[index];
        }

        auto erase(iterator where)
        {
            return children_.erase(where);
        }
        auto erase(iterator first, iterator last)
        {
            return children_.erase(first, last);
        }

        void clearChildren()
        {
            children_.clear();
        }

        bool hasChildren() const
        {
            return !children_.empty();
        }

        std::size_t childCount() const
        {
            return children_.size();
        }

        std::string tagName() const
        {
            return element_["tagName"].as<std::string>();
        }

      private:
        void replaceElementImpl(HtmlElement const& element)
        {
            clearChildren();
            if (unsetup_)
                unsetup_();
            unsetup_ = {};

#ifndef NDEBUG
            if (element_.isUndefined())
                throw std::runtime_error("Element is undefined");
#endif

            auto replacement = createElement(element);
            element_.call<Nui::val>("replaceWith", replacement);
            element_ = std::move(replacement);
            setup(element);
            if (deferredSetup_)
                deferredSetup_(element);
        }

      private:
        using destroy_fn = void (*)(Nui::val&);
        destroy_fn destroy_ = Detail::destroyByRemove;
        collection_type children_;
        std::function<void()> unsetup_;
        std::function<void(HtmlElement const& element)> deferredSetup_;
    };

    inline std::shared_ptr<Element> makeStandaloneElement(std::invocable<Element&, Renderer const&> auto&& fn)
    {
        auto elem = std::make_shared<Element>();
        fn(*elem, Renderer{.type = RendererType::Emplace});
        return elem;
    }
}

#include <nui/frontend/elements/impl/html_element.tpp>
#include <nui/frontend/elements/impl/range_renderer.tpp>