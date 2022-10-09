#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/utility/visit_overloaded.hpp>
#include <nui/elements/fragment.hpp>
#include <nui/elements/caption.hpp>
#include <nui/elements/table.hpp>
#include <nui/attributes/impl/custom_attribute.hpp>
#include <nui/generator_typedefs.hpp>
#include <nui/utility/meta/extract_value_type.hpp>

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
        NUI_MAKE_CUSTOM_ATTRIBUTE(tableAttributes);
        NUI_MAKE_CUSTOM_ATTRIBUTE(tableHeaderAttributes);
        NUI_MAKE_CUSTOM_ATTRIBUTE(tableBodyAttributes);
        NUI_MAKE_CUSTOM_ATTRIBUTE(tableFooterAttributes);
    }
    template <template <typename...> typename ContainerT, typename RowDataT>
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
                auto attribute = extractOptionalAttributePtr<tableCaptionTag>(args...);
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
            , headerRenderer_{extractOptionalAttributeFast<
                  headerRendererTag,
                  ExtractValueType_t<decltype(headerRenderer_)>>(args...)}
            , footerRenderer_{
                  extractOptionalAttributeFast<footerRendererTag, ExtractValueType_t<decltype(footerRenderer_)>>(
                      args...)}
        {}

      public:
        // Do not capture this in this function. Components are temporaray objects that produce functions which render
        // actual elements.
        template <typename... PassedArgs>
        constexpr auto operator()(PassedArgs&&... passedArgs) &&
        {
            using namespace Elements;

            // clang-format off
            return table{extractAttributeAsTuple<tableAttributesTag>(passedArgs...)}(
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
                [headerRenderer = std::move(headerRenderer_), &passedArgs...]() -> Nui::ElementRenderer {
                    if (headerRenderer)
                        return thead{extractAttributeAsTuple<tableHeaderAttributesTag>(passedArgs...)}((*headerRenderer)());
                    else
                        return nil();
                }(),
                // body
                tbody{extractAttributeAsTuple<tableBodyAttributesTag>(passedArgs...)}(
                    range(tableModel_),
                    [renderer = std::move(rowRenderer_)](long i, auto const& row) {
                        return tr{}(
                            renderer(i, row)
                        );
                    }
                ),
                // footer                
                [footerRenderer = std::move(footerRenderer_), &passedArgs...]() -> Nui::ElementRenderer {
                    if (footerRenderer)
                        return tfoot{extractAttributeAsTuple<tableFooterAttributesTag>(passedArgs...)}((*footerRenderer)());
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
        -> Table<ContainerT, RowDataT>;
}