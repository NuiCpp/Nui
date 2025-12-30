#include <nui/frontend/api/abort_signal.hpp>

namespace Nui::WebApi
{
    AbortSignal::AbortSignal(Nui::val val)
        : ValWrapper{std::move(val)}
    {}
    bool AbortSignal::aborted() const
    {
        return val_["aborted"].as<bool>();
    }
    Nui::val AbortSignal::reason() const
    {
        return val_["reason"];
    }
    void AbortSignal::abort() const
    {
        val_.call<void>("abort");
    }
    void AbortSignal::abort(Nui::val reason) const
    {
        val_.call<void>("abort", std::move(reason));
    }
    void AbortSignal::timeout(std::chrono::milliseconds time) const
    {
        val_.call<void>("timeout", time.count());
    }
}