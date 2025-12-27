#include <nui/frontend/api/abort_controller.hpp>

namespace Nui::WebApi
{
    AbortController::AbortController()
        : AbortController{Nui::val::global("AbortController").new_()}
    {}
    AbortController::AbortController(Nui::val val)
        : ValWrapper{std::move(val)}
    {}
    AbortSignal AbortController::signal()
    {
        return AbortSignal{val_["signal"]};
    }
    void AbortController::abort() const
    {
        val_.call<void>("abort");
    }
    void AbortController::abort(Nui::val reason) const
    {
        val_.call<void>("abort", std::move(reason));
    }
}