#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/utility/visit_overloaded.hpp>
#include <nui/elements/fragment.hpp>
#include <nui/elements/caption.hpp>
#include <nui/elements/table.hpp>
#include <nui/attributes/custom_attribute.hpp>
#include <nui/generator_typedefs.hpp>

// FIXME: removem me
#include <nui/elements/div.hpp>

#include <string>
#include <variant>

namespace Nui::Components
{
    inline namespace TableAttributes
    {
        NUI_MAKE_CUSTOM_ATTRIBUTE(tableModel);
        NUI_MAKE_CUSTOM_ATTRIBUTE(headerRenderer);
        NUI_MAKE_CUSTOM_ATTRIBUTE(footerRenderer);
        NUI_MAKE_CUSTOM_ATTRIBUTE(rowRenderer);
        NUI_MAKE_CUSTOM_ATTRIBUTE(tableCaption);
    }
    template <template <typename...> typename ContainerT, typename RowDataT, typename... ForwardedArgs>
    class Table
    {
      public:
        using TableModelType = ContainerT<RowDataT>;

      public:
        template <typename... Args>
        Table(CustomAttribute<Observed<ContainerT<RowDataT>>&, tableModelTag>&& model, Args&&... args)
            : tableModel_{model.get()}
            , captionModel_{[&]() -> decltype(captionModel_) {
                // TODO: I need a better pattern for this. optional<static or observed>
                auto attribute = extractOptionalAttribute<tableCaptionTag>(args...);
                if constexpr (std::is_same_v<Detail::InvalidAttribute* const, decltype(attribute)>)
                    return {};
                else if constexpr (std::is_same_v<std::string, decltype(attribute->get())>)
                    return attribute->get();
                else if constexpr (std::is_same_v<Observed<std::string>&, decltype(attribute->get())>)
                    return &attribute->get();
                else
                    return {};
            }()}
            , rowRenderer_{extractAttribute<rowRendererTag>(args...)}
            , headerRenderer_{[&]() -> decltype(headerRenderer_) {
                auto attribute = extractOptionalAttribute<headerRendererTag>(args...);
                if constexpr (std::is_same_v<Detail::InvalidAttribute* const, decltype(attribute)>)
                    return std::nullopt;
                else
                    return attribute->get();
            }()}
            , footerRenderer_{[&]() -> decltype(footerRenderer_) {
                auto attribute = extractOptionalAttribute<footerRendererTag>(args...);
                if constexpr (std::is_same_v<Detail::InvalidAttribute* const, decltype(attribute)>)
                    return std::nullopt;
                else
                    return attribute->get();
            }()}
        {}

      public:
        // Do not capture this in this function.
        // Rerenders might be caused after this short living class is destroyed.
        constexpr auto operator()() &&
        {
            // clang-format off
            return table{}(
                // caption
                [cap = std::move(captionModel_)]() -> Nui::ElementRenderer {
                    return visitOverloaded(cap,
                        [](std::monostate) -> Nui::ElementRenderer{
                            return nil();
                        },
                        [](std::string const& content) -> Nui::ElementRenderer{
                            return caption{}(content);
                        },
                        [](Observed<std::string>* model) -> Nui::ElementRenderer{
                            return caption{}(*model);
                        }
                    );
                }(),
                // header
                [headerRenderer = std::move(headerRenderer_)]() -> Nui::ElementRenderer {
                    if (headerRenderer)
                        return thead{}((*headerRenderer)());
                    else
                        return nil();
                }(),
                // body
                tbody{}(
                    range(tableModel_),
                    // careful: this is no longer valid on subsequent calls
                    [renderer = std::move(rowRenderer_)](long i, auto const& row) {
                        return tr{}(
                            renderer(i, row)
                        );
                    }
                ),
                // footer                
                [footerRenderer = std::move(footerRenderer_)]() -> Nui::ElementRenderer {
                    if (footerRenderer)
                        return thead{}((*footerRenderer)());
                    else
                        return nil();
                }()
            );
            // clang-format on
        }

      private:
        Observed<TableModelType>& tableModel_;
        std::variant<std::monostate, std::string, Observed<std::string>*> captionModel_;
        std::function<ElementRenderer(long, RowDataT const&)> rowRenderer_;
        std::optional<std::function<ElementRenderer()>> headerRenderer_;
        std::optional<std::function<ElementRenderer()>> footerRenderer_;
    };

    template <template <typename...> typename ContainerT, typename RowDataT, typename... Arguments>
    Table(CustomAttribute<Observed<ContainerT<RowDataT>>&, tableModelTag>&&, Arguments&&...)
        -> Table<ContainerT, RowDataT, Arguments...>;
}