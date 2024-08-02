namespace Nui
{
    class HtmlElement;

    namespace Materializers
    {
        /// Creates new actual element and makes it a child of the given parent.
        inline auto appendMaterialize(auto& parent, auto const& htmlElement)
        {
            return parent.appendElement(htmlElement);
        }
        /// Similar to appendMaterialize, but the new element is not added to the children list.
        /// Works together with inplaceMaterialize.
        inline auto fragmentMaterialize(auto& parent, auto const& htmlElement)
        {
            auto elem = parent.makeElement(htmlElement);
            parent.val().template call<Nui::val>("appendChild", elem->val());
            return elem;
        }
        /// Inserts new element at the given position of the given parent.
        inline auto insertMaterialize(std::size_t where, auto& parent, auto const& htmlElement)
        {
            return parent.insert(where, htmlElement);
        }
        /// Replaces the given element with the new one.
        inline auto replaceMaterialize(auto& element, auto const& htmlElement)
        {
            return element.replaceElement(htmlElement);
        }
        /// Replaces the given element with the new one.
        inline auto emplaceMaterialize(auto& element, auto const& htmlElement)
        {
            return element.emplaceElement(htmlElement);
        }
        /// Used for elements that dont have a direct parent.
        inline auto inplaceMaterialize(auto& element, auto const&)
        {
            return element.template shared_from_base<std::decay_t<decltype(element)>>();
        }
    }

    enum class RendererType
    {
        Append,
        Fragment,
        Insert,
        Replace,
        Inplace,
        Emplace
    };
    struct Renderer
    {
        RendererType type{RendererType::Append};
        std::size_t metadata{0};
    };
    auto renderElement(Renderer const& gen, auto& element, auto const& htmlElement)
    {
        switch (gen.type)
        {
            case RendererType::Append:
                return Materializers::appendMaterialize(element, htmlElement);
            case RendererType::Fragment:
                return Materializers::fragmentMaterialize(element, htmlElement);
            case RendererType::Insert:
                return Materializers::insertMaterialize(gen.metadata, element, htmlElement);
            case RendererType::Replace:
                return Materializers::replaceMaterialize(element, htmlElement);
            case RendererType::Inplace:
                return Materializers::inplaceMaterialize(element, htmlElement);
            case RendererType::Emplace:
                return Materializers::emplaceMaterialize(element, htmlElement);
        }
    };
}