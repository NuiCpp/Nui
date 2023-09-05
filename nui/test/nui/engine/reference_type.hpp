#pragma once

#include <compare>

namespace Nui::Tests::Engine
{
    class ReferenceType
    {
      public:
        explicit ReferenceType(long long id)
            : id_{id}
        {}
        ReferenceType() = default;
        ReferenceType(const ReferenceType&) = default;
        ReferenceType(ReferenceType&&) = default;
        ReferenceType& operator=(const ReferenceType&) = default;

        ReferenceType& operator=(ReferenceType&&) = default;
        auto operator<=>(const ReferenceType&) const = default;

        operator long long() const
        {
            return id_;
        }
        long long id() const
        {
            return id_;
        }
        std::size_t uid() const
        {
            return static_cast<std::size_t>(id_);
        }

        ReferenceType operator++()
        {
            return ReferenceType{++id_};
        }
        ReferenceType operator++(int)
        {
            return ReferenceType{id_++};
        }

      private:
        long long id_;
    };

    inline ReferenceType invalidReference()
    {
        return ReferenceType{-1};
    }
}