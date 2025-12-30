#pragma once

#include <nui/frontend/val.hpp>

namespace Nui
{
    /**
     * @brief Common base class for classes that are just wrapping around val into js world.
     */
    class ValWrapper
    {
      public:
        explicit ValWrapper(Nui::val valObject);
        virtual ~ValWrapper() = default;
        ValWrapper(ValWrapper const&) = default;
        ValWrapper(ValWrapper&&) noexcept = default;
        ValWrapper& operator=(ValWrapper const&) = default;
        ValWrapper& operator=(ValWrapper&&) noexcept = default;

        Nui::val const& val() const& noexcept;
        Nui::val val() && noexcept;
        explicit operator Nui::val const&() const& noexcept;
        explicit operator Nui::val() && noexcept;

      protected:
        Nui::val val_;
    };
}