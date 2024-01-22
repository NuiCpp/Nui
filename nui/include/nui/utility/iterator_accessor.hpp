#pragma once

#include <type_traits>

namespace Nui
{
    template <typename ContainerT>
    struct IteratorAccessor
    {
      public:
        using IteratorType = typename ContainerT::iterator;
        using ConstIteratorType = typename ContainerT::const_iterator;

        explicit IteratorAccessor(ContainerT& container)
            : container_{&container}
        {}
        ~IteratorAccessor() = default;
        IteratorAccessor(IteratorAccessor const&) = default;
        IteratorAccessor(IteratorAccessor&&) = default;
        IteratorAccessor& operator=(IteratorAccessor const&) = default;
        IteratorAccessor& operator=(IteratorAccessor&&) = default;

        ConstIteratorType begin() const
        requires(std::is_const_v<ContainerT>)
        {
            return container_->begin();
        }

        ConstIteratorType end() const
        requires(std::is_const_v<ContainerT>)
        {
            return container_->end();
        }

        IteratorType begin() const
        requires(!std::is_const_v<ContainerT>)
        {
            return container_->begin();
        }

        IteratorType end() const
        requires(!std::is_const_v<ContainerT>)
        {
            return container_->end();
        }

      private:
        ContainerT* container_;
    };

    // Deduction guide for const ContainerT
    template <typename ContainerT>
    IteratorAccessor(ContainerT const&) -> IteratorAccessor<const ContainerT>;

    // Deduction guide for non-const ContainerT
    template <typename ContainerT>
    IteratorAccessor(ContainerT&) -> IteratorAccessor<ContainerT>;
}