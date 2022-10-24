#include "throttle.hpp"

#include <nui/data_structures/selectables_registry.hpp>

#include <boost/asio/deadline_timer.hpp>

#include <chrono>
#include <memory>
#include <limits>
#include <mutex>

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace Nui
{
    namespace Detail
    {
        constexpr static char const* throttleStoreId = "ThrottleStore";

        class ThrottleInstance : public std::enable_shared_from_this<ThrottleInstance>
        {
          public:
            ThrottleInstance(
                std::chrono::milliseconds interval,
                bool callWhenReady,
                RpcHub* hub,
                boost::asio::any_io_executor executor)
                : guard_{}
                , interval_{interval}
                , lastCallTime_{std::chrono::high_resolution_clock::now() - 2 * interval}
                , timer_{std::move(executor)}
                , callWhenReady_{callWhenReady}
                , timerIsRunning_{false}
                , hub_{hub}
                , id_{std::numeric_limits<decltype(id_)>::max()}
            {}
            ~ThrottleInstance()
            {
                std::scoped_lock lock{guard_};
                timer_.cancel();
            }

            bool mayCall()
            {
                std::scoped_lock lock{guard_};
                const auto now = std::chrono::high_resolution_clock::now();
                const auto timeSinceLastCall = now - lastCallTime_;
                if (timeSinceLastCall > interval_)
                {
                    lastCallTime_ = now;
                    return true;
                }
                else
                {
                    if (callWhenReady_ && !timerIsRunning_)
                    {
                        timerIsRunning_ = true;
                        const auto waitingTime =
                            std::chrono::duration_cast<std::chrono::milliseconds>(interval_ - timeSinceLastCall);

                        timer_.expires_from_now(boost::posix_time::milliseconds(waitingTime.count()));
                        timer_.async_wait([weak = weak_from_this()](const boost::system::error_code& error) {
                            if (error)
                                return;
                            if (auto shared = weak.lock())
                            {
                                std::scoped_lock lock{shared->guard_};
                                shared->timerIsRunning_ = false;
                                shared->hub_->callRemote(shared->throttledCallWhenReadyWithId_);
                                shared->lastCallTime_ = std::chrono::high_resolution_clock::now();
                            }
                        });
                    }
                    return false;
                }
            }

            void setId(Nui::SelectablesRegistry<ThrottleInstance>::IdType id)
            {
                std::scoped_lock lock{guard_};
                id_ = id;
                throttledCallWhenReadyWithId_ = "Nui::throttledCallWhenReady_"s + std::to_string(id_);
            }

          private:
            std::recursive_mutex guard_;
            std::chrono::milliseconds interval_;
            std::chrono::high_resolution_clock::time_point lastCallTime_;
            boost::asio::deadline_timer timer_;
            bool callWhenReady_;
            bool timerIsRunning_;
            Nui::RpcHub* hub_;
            Nui::SelectablesRegistry<ThrottleInstance>::IdType id_;
            std::string throttledCallWhenReadyWithId_;
        };
        using ThrottleStore = SelectablesRegistry<std::shared_ptr<ThrottleInstance>>;

        struct ThrottleStoreCreator
        {
            static void* create()
            {
                return new ThrottleStore();
            }
            static void destroy(void* ptr)
            {
                delete static_cast<ThrottleStore*>(ptr);
            }
        };

        ThrottleStore& getStore(auto& hub)
        {
            return *static_cast<ThrottleStore*>(hub.template accessStateStore<ThrottleStoreCreator>(throttleStoreId));
        }
    }

    void registerThrottle(Nui::RpcHub& hub)
    {
        hub.registerFunction(
            "Nui::throttle", [&hub](std::string const& responseId, int32_t period, bool callWhenReady) {
                auto& store = Detail::getStore(hub);
                const auto id = store.append(std::make_shared<Detail::ThrottleInstance>(
                    std::chrono::milliseconds(period), callWhenReady, &hub, hub.window().getExecutor()));
                (*store[id].item)->setId(id);
                hub.callRemote(responseId, id);
            });
        hub.registerFunction("Nui::removeThrottle", [&hub](int32_t id) {
            auto& store = Detail::getStore(hub);
            store.erase(static_cast<Nui::SelectablesRegistry<Detail::ThrottleInstance>::IdType>(id));
        });
        hub.registerFunction("Nui::mayCallThrottled", [&hub](std::string const& responseId, int32_t id) {
            auto& store = Detail::getStore(hub);
            auto& instance = *store[static_cast<Nui::SelectablesRegistry<Detail::ThrottleInstance>::IdType>(id)].item;
            hub.callRemote(responseId, instance->mayCall());
        });
    }
}