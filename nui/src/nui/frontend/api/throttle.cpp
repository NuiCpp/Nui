#include <nui/frontend/api/throttle.hpp>

using namespace std::string_literals;

namespace Nui
{
    // #####################################################################################################################
    ThrottledFunction::ThrottledFunction()
        : id_{-1}
        , calledWhenReady_{false}
        , func_{}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    ThrottledFunction::ThrottledFunction(int32_t id, bool calledWhenReady, std::function<void()> func)
        : id_{id}
        , calledWhenReady_{calledWhenReady}
        , func_{std::move(func)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    ThrottledFunction::~ThrottledFunction()
    {
        if (id_ != -1)
        {
            RpcClient::getRemoteCallable("Nui::removeThrottle")(id_);
            if (calledWhenReady_)
                RpcClient::unregisterFunction("Nui::throttledCallWhenReady_"s + std::to_string(id_));
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    ThrottledFunction::ThrottledFunction(ThrottledFunction&& other)
        : id_(other.id_)
        , func_(std::move(other.func_))
    {
        other.id_ = -1;
    }
    //---------------------------------------------------------------------------------------------------------------------
    ThrottledFunction& ThrottledFunction::operator=(ThrottledFunction&& other)
    {
        id_ = other.id_;
        func_ = std::move(other.func_);
        other.id_ = -1;
        return *this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ThrottledFunction::operator()()
    {
        if (id_ != -1 && func_)
            func_();
    }
    // #####################################################################################################################
    void throttle(
        int milliseconds,
        std::function<void()> toWrap,
        std::function<void(ThrottledFunction&&)> callback,
        bool callWhenReady)
    {
        RpcClient::getRemoteCallableWithBackChannel(
            "Nui::throttle",
            [callback = std::move(callback), toWrap = std::move(toWrap), callWhenReady](int32_t throttleId) {
                if (callWhenReady)
                {
                    RpcClient::registerFunction(
                        "Nui::throttledCallWhenReady_"s + std::to_string(throttleId), [toWrap]() {
                            toWrap();
                        });
                }

                callback(ThrottledFunction{
                    throttleId,
                    callWhenReady,
                    [throttleId, toWrap]() {
                        RpcClient::getRemoteCallableWithBackChannel("Nui::mayCallThrottled", [&toWrap](bool mayCall) {
                            if (mayCall)
                                toWrap();
                        })(throttleId);
                    },
                });
            })(milliseconds, callWhenReady);
    }
    // #####################################################################################################################
}