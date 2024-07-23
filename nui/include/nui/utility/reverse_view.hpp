#pragma once

#include <iterator>

namespace Nui
{
    template <typename T>
    class reverse_view
    {
      public:
        reverse_view(T& container)
            : container_{container}
        {}

        auto begin() const
        {
            return std::rbegin(container_);
        }

        auto end() const
        {
            return std::rend(container_);
        }

      private:
        T& container_;
    };
}