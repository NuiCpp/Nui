#pragma once

#include <functional>

#include <nui/frontend/rpc_client.hpp>

namespace Nui
{
    class ThrottledFunction
    {
      public:
        ThrottledFunction();
        ThrottledFunction(int32_t id, bool calledWhenReady, std::function<void()> func);
        ~ThrottledFunction();
        ThrottledFunction(ThrottledFunction const&) = delete;
        ThrottledFunction(ThrottledFunction&& other);
        ThrottledFunction& operator=(ThrottledFunction const&) = delete;
        ThrottledFunction& operator=(ThrottledFunction&& other);
        void operator()();

      private:
        int32_t id_{-1};
        bool calledWhenReady_;
        std::function<void()> func_{};
    };

    /**
     * @brief Creates a function that can be used to call a function at most once during the specified interval.
     * You can imagine it as a std::call_once that is reset after the given time.
     * This function is not very accurate, because of the back and forth travel time.
     *
     * @param milliseconds The minimum time between each call of the function.
     * @param func The function to call throttled.
     * @param callback This callback receives the throttled function.
     * @param callWhenReady call the function when ready if it was not during the wait interval.
     */
    void throttle(
        int milliseconds,
        std::function<void()> toWrap,
        std::function<void(ThrottledFunction&&)> callback,
        bool callWhenReady = false);
}