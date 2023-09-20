#include "throttle.hpp"

#include <nui/data_structures/selectables_registry.hpp>

#include <boost/asio/high_resolution_timer.hpp>

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
        constexpr static char const* timerStoreId = "TimerStore";

        class TimerInstance;

        void eraseTimerInstance(Nui::RpcHub* hub, int32_t id);

        class TimerInstance : public std::enable_shared_from_this<TimerInstance>
        {
          public:
            TimerInstance(
                std::chrono::milliseconds interval,
                RpcHub* hub,
                boost::asio::any_io_executor executor,
                int callLimit)
                : guard_{}
                , interval_{interval}
                , timer_{std::move(executor)}
                , hub_{hub}
                , id_{std::numeric_limits<decltype(id_)>::max()}
                , remoteName_{}
                , callLimit_{callLimit}
            {}
            ~TimerInstance()
            {
                std::scoped_lock lock{guard_};
                timer_.cancel();
            }

            void start()
            {
                timer_.expires_after(interval_);
                timer_.async_wait([weak = weak_from_this()](const boost::system::error_code& error) {
                    if (error)
                        return;
                    if (auto shared = weak.lock())
                        shared->onTimer();
                });
            }

            void setId(Nui::SelectablesRegistry<TimerInstance>::IdType id)
            {
                std::scoped_lock lock{guard_};
                id_ = id;
                remoteName_ = "Nui::timerCall_"s + std::to_string(id_);
            }

          private:
            void onTimer()
            {
                std::scoped_lock lock{guard_};
                hub_->callRemote(remoteName_);
                if (callLimit_ > 0)
                {
                    if (--callLimit_ == 0)
                    {
                        eraseTimerInstance(hub_, static_cast<int32_t>(id_));
                        return;
                    }
                }
                start();
            }

          private:
            std::recursive_mutex guard_;
            std::chrono::milliseconds interval_;
            boost::asio::high_resolution_timer timer_;
            Nui::RpcHub* hub_;
            Nui::SelectablesRegistry<TimerInstance>::IdType id_;
            std::string remoteName_;
            int callLimit_;
        };
        using TimerStore = SelectablesRegistry<std::shared_ptr<TimerInstance>>;

        struct TimerStoreCreator
        {
            static void* create()
            {
                return new TimerStore();
            }
            static void destroy(void* ptr)
            {
                delete static_cast<TimerStore*>(ptr);
            }
        };

        TimerStore& getStore(auto& hub)
        {
            return *static_cast<TimerStore*>(hub.template accessStateStore<TimerStoreCreator>(timerStoreId));
        }

        void eraseTimerInstance(Nui::RpcHub* hub, int32_t id)
        {
            auto& store = Detail::getStore(*hub);
            store.erase(static_cast<Nui::SelectablesRegistry<Detail::TimerInstance>::IdType>(id));
        }
    }

    void registerTimer(Nui::RpcHub& hub)
    {
        hub.registerFunction("Nui::setInterval", [&hub](std::string const& responseId, int32_t period) {
            auto& store = Detail::getStore(hub);
            const auto id = store.append(std::make_shared<Detail::TimerInstance>(
                std::chrono::milliseconds(period), &hub, hub.window().getExecutor(), -1));
            auto& timer = store[id];
            timer->setId(id);
            timer->start();
            hub.callRemote(responseId, id);
        });
        hub.registerFunction("Nui::setTimeout", [&hub](std::string const& responseId, int32_t period) {
            auto& store = Detail::getStore(hub);
            const auto id = store.append(std::make_shared<Detail::TimerInstance>(
                std::chrono::milliseconds(period), &hub, hub.window().getExecutor(), 1));
            store[id]->setId(id);
            auto& timer = store[id];
            timer->setId(id);
            timer->start();
            hub.callRemote(responseId, id);
        });
        hub.registerFunction("Nui::removeTimer", [&hub](int32_t id) {
            Detail::eraseTimerInstance(&hub, id);
        });
    }
}