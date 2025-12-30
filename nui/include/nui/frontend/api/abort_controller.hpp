#pragma once

#include <nui/frontend/api/abort_signal.hpp>

#include <nui/frontend/val_wrapper.hpp>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/AbortController
     */
    class AbortController : public ValWrapper
    {
      public:
        AbortController();
        explicit AbortController(Nui::val val);

        AbortSignal signal();

        /**
         * @brief Aborts an asynchronous operation before it has completed. This is able to abort fetch requests,
         * consumption of any response bodies, and streams.
         */
        void abort() const;

        /**
         * @brief Aborts an asynchronous operation before it has completed. This is able to abort fetch requests,
         * consumption of any response bodies, and streams.
         *
         * @param reason The reason why the operation was aborted, which can be any JavaScript value. If not specified,
         * the reason is set to "AbortError" DOMException.
         */
        void abort(Nui::val reason) const;
    };
}