#pragma once

#include <nui/frontend/val_wrapper.hpp>

#include <chrono>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/AbortSignal
     */
    class AbortSignal : public ValWrapper
    {
      public:
        explicit AbortSignal(Nui::val val);

        /**
         * @brief A Boolean that indicates whether the request(s) the signal is communicating with is/are aborted (true)
         * or not (false).
         */
        bool aborted() const;

        /**
         * @brief A JavaScript value providing the abort reason, once the signal has aborted.
         */
        Nui::val reason() const;

        /**
         * @brief The AbortSignal.abort() static method returns an AbortSignal that is already set as aborted (and which
         * does not trigger an abort event).
         */
        void abort() const;

        /**
         * @brief The AbortSignal.abort() static method returns an AbortSignal that is already set as aborted (and which
         * does not trigger an abort event).
         *
         * @param reason The reason why the operation was aborted, which can be any JavaScript value. If not specified,
         * the reason is set to "AbortError" DOMException.
         */
        void abort(Nui::val reason) const;

        /**
         * @brief The AbortSignal.timeout() static method returns an AbortSignal that will automatically abort after a
         * specified time. The signal aborts with a TimeoutError DOMException on timeout.
         *
         * @param time A time to timeout on.
         */
        void timeout(std::chrono::milliseconds time) const;
    };
}