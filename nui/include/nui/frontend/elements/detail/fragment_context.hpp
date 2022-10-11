#pragma once

namespace Nui::Detail
{
    template <typename ElementT>
    class FragmentContext
    {
      public:
        void clear()
        {
            fragmentElements_.clear();
        }

        void push(std::shared_ptr<ElementT> weak)
        {
            fragmentElements_.emplace_back(std::move(weak));
        }

      private:
        std::vector<std::shared_ptr<ElementT>> fragmentElements_;
    };
}