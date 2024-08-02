#pragma once

#include <type_traits>
#include <memory>
namespace Nui
{
    template <typename ContainerT>
    class IteratorAccessor
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

    template <typename ContainerT>
    class IteratorAccessor<std::weak_ptr<ContainerT>>
    {
      public:
        using IteratorType = typename ContainerT::iterator;
        using ConstIteratorType = typename ContainerT::const_iterator;

        explicit IteratorAccessor(std::weak_ptr<ContainerT> container)
            : container_{std::move(container)}
        {}
        ~IteratorAccessor() = default;
        IteratorAccessor(IteratorAccessor const&) = default;
        IteratorAccessor(IteratorAccessor&&) = default;
        IteratorAccessor& operator=(IteratorAccessor const&) = default;
        IteratorAccessor& operator=(IteratorAccessor&&) = default;

        ConstIteratorType begin() const
        requires(std::is_const_v<ContainerT>)
        {
            return container_.lock()->begin();
        }

        ConstIteratorType end() const
        requires(std::is_const_v<ContainerT>)
        {
            return container_.lock()->end();
        }

        IteratorType begin() const
        requires(!std::is_const_v<ContainerT>)
        {
            return container_.lock()->begin();
        }

        IteratorType end() const
        requires(!std::is_const_v<ContainerT>)
        {
            return container_.lock()->end();
        }

      private:
        std::weak_ptr<ContainerT> container_;
    };

    template <typename ContainerT>
    class IteratorAccessor<std::shared_ptr<ContainerT>>
    {
      public:
        using IteratorType = typename ContainerT::iterator;
        using ConstIteratorType = typename ContainerT::const_iterator;

        explicit IteratorAccessor(std::weak_ptr<ContainerT> container)
            : container_{std::move(container)}
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
        std::shared_ptr<ContainerT> container_;
    };

    // Deduction guide for const ContainerT
    template <typename ContainerT>
    IteratorAccessor(ContainerT const&) -> IteratorAccessor<const ContainerT>;

    // Deduction guide for non-const ContainerT
    template <typename ContainerT>
    IteratorAccessor(ContainerT&) -> IteratorAccessor<ContainerT>;
}