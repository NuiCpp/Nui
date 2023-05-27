#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/utility/visit_overloaded.hpp>
#include <nui/frontend/elements/fragment.hpp>
#include <nui/frontend/elements/caption.hpp>
#include <nui/frontend/elements/table.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/generator_typedefs.hpp>

#include <string>
#include <optional>
#include <functional>
#include <memory>
#include <vector>

namespace Nui::Components
{
    template <template <typename...> typename ContainerT, typename ElementT, typename... OtherArgs>
    struct TableArguments
    {
        Nui::Observed<ContainerT<ElementT, OtherArgs...>>& tableModel;
        std::variant<std::monostate, std::string, Nui::Observed<std::string> const*> caption = std::monostate{};
        std::function<Nui::ElementRenderer()> headerRenderer = {};
        std::function<Nui::ElementRenderer()> footerRenderer = {};
        std::function<Nui::ElementRenderer(long long i, ElementT const&)> rowRenderer = {};
        std::vector<Attribute> tableAttributes = {};
        std::vector<Attribute> headerAttributes = {};
        std::vector<Attribute> bodyAttributes = {};
        std::vector<Attribute> footerAttributes = {};
    };

    template <typename ModelT>
    class Table : public Table<std::vector<ModelT>>
    {
        using Table<std::vector<ModelT>>::Table;
    };
    template <template <typename...> typename ContainerT, typename ElementT, typename... OtherArgs>
    class Table<ContainerT<ElementT, OtherArgs...>>
    {
      public:
        constexpr Table(TableArguments<ContainerT, ElementT, OtherArgs...>&& args)
            : tableParams_{std::move(args)}
        {}

        inline Nui::ElementRenderer operator()() &&
        {
            using namespace Elements;

            // clang-format off
            return table{tableParams_.tableAttributes}(
                // caption
                [cap = std::move(tableParams_.caption)]() -> Nui::ElementRenderer {
                    return visitOverloaded(cap,
                        [](std::monostate) -> Nui::ElementRenderer{
                            return nil();
                        },
                        [](std::string const& content) -> Nui::ElementRenderer{
                            return caption{}(content);
                        },
                        [](Observed<std::string> const* model) -> Nui::ElementRenderer{
                            return caption{}(*model);
                        }
                    );
                }(),
                // header
                [headerRenderer = std::move(tableParams_.headerRenderer), headerAttributes = std::move(tableParams_.headerAttributes)]() -> Nui::ElementRenderer {
                    if (headerRenderer)
                        return thead{headerAttributes}(headerRenderer());
                    else
                        return nil();
                }(),
                // body
                tbody{tableParams_.bodyAttributes}(
                    range(tableParams_.tableModel),
                    [renderer = std::move(tableParams_.rowRenderer)](long i, auto const& row) -> Nui::ElementRenderer {
                        using namespace std::string_literals;

                        if (renderer)
                            return renderer(i, row);
                        else
                            return tr{}(td{}("NUI_MISSING_TABLE_RENDERER_"s + std::to_string(i)));
                    }
                ),
                // footer
                [footerRenderer = std::move(tableParams_.footerRenderer), footerAttributes = std::move(tableParams_.footerAttributes)]() -> Nui::ElementRenderer {
                    if (footerRenderer)
                        return
                        tfoot{footerAttributes}(footerRenderer());
                    else
                        return nil();
                }()
            );
            // clang-format on
        }

      private:
        TableArguments<ContainerT, ElementT, OtherArgs...> tableParams_;
    };

    template <template <typename...> typename ContainerT, typename ElementT, typename... OtherArgs>
    Table(TableArguments<ContainerT, ElementT, OtherArgs...>&& args) -> Table<ContainerT<ElementT, OtherArgs...>>;
}