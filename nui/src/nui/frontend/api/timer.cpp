#include <nui/frontend/api/timer.hpp>

using namespace std::string_literals;

namespace Nui
{
    // #####################################################################################################################
    TimerHandle::TimerHandle()
        : id_{-1}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    TimerHandle::TimerHandle(int32_t id)
        : id_{id}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    TimerHandle::~TimerHandle()
    {
        stop();
    }
    //---------------------------------------------------------------------------------------------------------------------
    TimerHandle::TimerHandle(TimerHandle&& other) noexcept
        : id_{other.id_}
    {
        other.id_ = -1;
    }
    //---------------------------------------------------------------------------------------------------------------------
    TimerHandle& TimerHandle::operator=(TimerHandle&& other) noexcept
    {
        id_ = other.id_;
        other.id_ = -1;
        return *this;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void TimerHandle::stop()
    {
        if (id_ != -1)
        {
            RpcClient::getRemoteCallable("Nui::removeTimer")(id_);
            RpcClient::unregisterFunction("Nui::timerCall_"s + std::to_string(id_));
            id_ = -1;
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool TimerHandle::hasActiveTimer() const
    {
        return id_ != -1;
    }
    // #####################################################################################################################
    void setInterval(int milliseconds, std::function<void()> toWrap, std::function<void(TimerHandle&&)> callback)
    {
        RpcClient::getRemoteCallableWithBackChannel(
            "Nui::setInterval", [callback = std::move(callback), toWrap = std::move(toWrap)](int32_t timerId) {
                RpcClient::registerFunction("Nui::timerCall_"s + std::to_string(timerId), [toWrap]() {
                    toWrap();
                });
                callback(TimerHandle{timerId});
            })(milliseconds);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void setTimeout(int milliseconds, std::function<void()> toWrap, std::function<void(TimerHandle)> callback)
    {
        RpcClient::getRemoteCallableWithBackChannel(
            "Nui::setTimeout", [callback = std::move(callback), toWrap = std::move(toWrap)](int32_t timerId) {
                RpcClient::registerFunction("Nui::timerCall_"s + std::to_string(timerId), [toWrap, timerId]() {
                    toWrap();
                    RpcClient::unregisterFunction("Nui::timerCall_"s + std::to_string(timerId));
                });
                callback(TimerHandle{timerId});
            })(milliseconds);
    }
    // #####################################################################################################################
}