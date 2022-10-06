#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/utility/visit_overloaded.hpp>
#include <nui/elements/fragment.hpp>
#include <nui/attributes/custom_attribute.hpp>
#include <nui/generator_typedefs.hpp>

// FIXME: removem me
#include <nui/elements/div.hpp>

#include <string>
#include <variant>

namespace Nui::Components
{
    NUI_MAKE_CUSTOM_ATTRIBUTE(tableModel)
    NUI_MAKE_CUSTOM_ATTRIBUTE(headerRenderer)
    NUI_MAKE_CUSTOM_ATTRIBUTE(rowRenderer)

    template <template <typename...> typename ContainerT, typename RowDataT, typename... ForwardedArgs>
    class Table
    {
      public:
        using TableModelType = ContainerT<RowDataT>;

      public:
        template <typename... Args>
        Table(CustomAttribute<Observed<ContainerT<RowDataT>>&, tableModelTag>&& model, Args&&... args)
            : tableModel_{model.get()}
            , rowRenderer_{extractAttribute<rowRendererTag>(args...)}
            , headerRenderer_{extractAttribute<headerRendererTag>(args...)}
        {}

      public:
        constexpr auto operator()() &&
        {
            // clang-format off
            return table{}(
                thead{}(
                    headerRenderer_()
                ),
                tbody{}(
                    range(tableModel_),
                    // careful: this is no longer valid on subsequent calls
                    [renderer = rowRenderer_](long i, auto const& row) {
                        return tr{}(
                            renderer(i, row)
                        );
                    }
                )
            );
            // clang-format on

            // return std::apply(
            //     [&](auto&&... tableArgs) {
            //         using Nui::table;
            //         using Nui::thead;
            //         using Nui::tbody;
            //         namespace attr = Nui::Attributes;
            //         using namespace attr::Literals;

            //         // clang-format off
            //         return table{
            //         }(
            //             fragment(
            //                 [this]() -> Nui::AppendGenerator {
            //                     return visitOverloaded(captionModel_,
            //                         [](std::monostate){
            //                             return nil();
            //                         },
            //                         [](std::string const& content){
            //                             return caption{}(content);
            //                         },
            //                         [](Observed<std::string>& model){
            //                             return caption{}(
            //                                 observe(model), [&model](){return *model;}
            //                             );
            //                         }
            //                     );
            //                 }
            //             ),
            //             thead{}(

            //             )
            //         );
            //         // clang-format on
            //     },
            //     tableArgs_);
        }

      private:
        // std::variant<std::monostate, std::string, Observed<std::string>*> captionModel_;
        Observed<TableModelType>& tableModel_;
        std::function<ElementGenerator(long, RowDataT const&)> rowRenderer_;
        std::function<ElementGenerator()> headerRenderer_;
    };

    template <template <typename...> typename ContainerT, typename RowDataT, typename... Arguments>
    Table(CustomAttribute<Observed<ContainerT<RowDataT>>&, tableModelTag>&&, Arguments&&...)
        -> Table<ContainerT, RowDataT, Arguments...>;
}