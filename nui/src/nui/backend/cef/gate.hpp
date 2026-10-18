#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace Nui::Detail::Cef
{
    /**
     * @brief Counts calls into a window from engine threads, so the window can wait for them before it is destroyed.
     */
    class Gate
    {
      public:
        /**
         * @brief Enters the gate. Returns false once the gate is closed, the caller must not touch the window then.
         */
        bool enter()
        {
            std::scoped_lock lock{mutex_};
            if (closed_)
                return false;
            ++inFlight_;
            return true;
        }

        void leave()
        {
            std::scoped_lock lock{mutex_};
            if (--inFlight_ == 0)
                drained_.notify_all();
        }

        /**
         * @brief Closes the gate and waits until every entered call has left, at most for the timeout.
         *
         * @param ownEntries Entries held by the calling thread itself, which cannot leave while it waits.
         */
        bool closeAndWait(std::chrono::milliseconds timeout, int ownEntries)
        {
            std::unique_lock lock{mutex_};
            closed_ = true;
            return drained_.wait_for(lock, timeout, [this, ownEntries]() {
                return inFlight_ <= ownEntries;
            });
        }

        bool closed() const
        {
            std::scoped_lock lock{mutex_};
            return closed_;
        }

      private:
        mutable std::mutex mutex_{};
        std::condition_variable drained_{};
        int inFlight_{0};
        bool closed_{false};
    };

    /**
     * @brief RAII entry of a Gate. Converts to false when the gate was already closed.
     */
    class GateEntry
    {
      public:
        explicit GateEntry(Gate& gate);
        ~GateEntry();
        GateEntry(GateEntry const&) = delete;
        GateEntry& operator=(GateEntry const&) = delete;
        GateEntry(GateEntry&&) = delete;
        GateEntry& operator=(GateEntry&&) = delete;

        explicit operator bool() const
        {
            return entered_;
        }

        /**
         * @brief Number of entries the current thread holds on the gate.
         */
        static int heldByCurrentThread(Gate const& gate);

      private:
        Gate* gate_;
        bool entered_;
    };
}
