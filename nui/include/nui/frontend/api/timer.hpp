#pragma once

#include <functional>

#include <nui/frontend/rpc_client.hpp>

#include <cstdint>

namespace Nui
{
    class TimerHandle
    {
      public:
        TimerHandle();
        TimerHandle(int32_t id);
        TimerHandle(const TimerHandle&) = delete;
        TimerHandle(TimerHandle&& other);
        TimerHandle& operator=(const TimerHandle&) = delete;
        TimerHandle& operator=(TimerHandle&& other);

        ~TimerHandle();

        void stop();
        bool hasActiveTimer() const;

      private:
        int32_t id_;
    };

    /**
     * @brief Creates a new timer that calls "toWrap" every "milliseconds" milliseconds.
     * This function is less accurate than the browser setInterval, because it has to traverse from backend to frontend.
     * This is an alternative if you have trouble with ASYNCIFY and code running from setInterval.
     * You should avoid using this function for small time intervals to avoid performance issues.
     *
     * @param milliseconds The time between each call of the function.
     * @param toWrap The function to call periodically.
     * @param callback This callback receives the interval handle. The timer can be stopped using this handle.
     */
    void setInterval(int milliseconds, std::function<void()> toWrap, std::function<void(TimerHandle&&)> callback);

    /**
     * @brief Creates a new delayed function that calls "toWrap" after "milliseconds" milliseconds.
     * This function is less accurate than the browser setTimeout, because it has to traverse from backend to frontend.
     * This is an alternative if you have trouble with ASYNCIFY and code running from setTimeout.
     * You should avoid using this function for small timeouts to avoid performance issues.
     *
     * @param milliseconds The time to wait before calling the function.
     * @param toWrap The function to call after the timeout.
     * @param callback This callback receives the timeout handle. The timeout can be stopped using this handle.
     */
    void setTimeout(int milliseconds, std::function<void()> toWrap, std::function<void(TimerHandle)> callback);
}