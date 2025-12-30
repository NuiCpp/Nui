#include <nui/frontend/val_wrapper.hpp>

namespace Nui
{
    ValWrapper::ValWrapper(Nui::val valObject)
        : val_{std::move(valObject)}
    {}
    Nui::val const& ValWrapper::val() const& noexcept
    {
        return val_;
    }
    Nui::val ValWrapper::val() && noexcept
    {
        return std::move(val_);
    }
    ValWrapper::operator Nui::val const&() const& noexcept
    {
        return val_;
    }
    ValWrapper::operator Nui::val() && noexcept
    {
        return std::move(val_);
    }
}