#pragma once

#include <nui/event_system/listen.hpp>
#include <nui/event_system/tags.hpp>
#include <nui/rpc.hpp>

#ifndef NUI_FRONTEND
#    include <nlohmann/json.hpp>
#else
#    include <nui/frontend/utility/val_conversion.hpp>
#endif

#include <memory>

namespace Nui
{
    template <typename ObservedT>
    class SynchronizerBase
    {
      public:
#ifdef NUI_FRONTEND
        using AutoUnregister = RpcClient::AutoUnregister;
#else
        using AutoUnregister = RpcHub::AutoUnregister;
#endif
        AutoUnregister unregister;
        ListenRemover<ObservedT> listenRemover;

        SynchronizerBase(AutoUnregister&& unregister, ListenRemover<ObservedT>&& listenRemover)
            : unregister{std::move(unregister)}
            , listenRemover{std::move(listenRemover)}
        {}
    };

    template <typename ObservedT>
    class Synchronizer : public SynchronizerBase<ObservedT>
    {
#ifdef NUI_FRONTEND

      private:
        explicit Synchronizer(ObservedT& observed, std::shared_ptr<bool> const& eventComesFromRpc)
            : SynchronizerBase<ObservedT>{
                  RpcClient::autoRegisterFunction(
                      "observed_" + std::string(syncId(observed)),
                      [&observed, eventComesFromRpc = eventComesFromRpc](Nui::val value) mutable {
                          {
                              *eventComesFromRpc = true;
                              convertFromVal(value, observed.modifyNow().value());
                          }
                          *eventComesFromRpc = false;
                      }),
                  Nui::smartListen(
                      observed,
                      [eventComesFromRpc = eventComesFromRpc](auto const& value) mutable {
                          if (!*eventComesFromRpc)
                              RpcClient::call("observed_" + std::string(syncId(ObservedT{})), convertToVal(value));
                      }),
              }
        {}

      public:
        explicit Synchronizer(ObservedT& observed)
            : Synchronizer{observed, std::make_shared<bool>(false)}
        {}
#else

      private:
        explicit Synchronizer(RpcHub& hub, ObservedT& observed, std::shared_ptr<bool> const& eventComesFromRpc)
            : SynchronizerBase<ObservedT>{
                  hub.autoRegisterFunction(
                      "observed_" + std::string(syncId(observed)),
                      [&observed, eventComesFromRpc = eventComesFromRpc](nlohmann::json const& value) mutable {
                          {
                              *eventComesFromRpc = true;
                              observed = value.template get<typename ObservedT::observed_type>();
                              observed.eventContext().sync();
                          }
                          *eventComesFromRpc = false;
                      }),
                  Nui::smartListen(
                      observed,
                      [&hub, eventComesFromRpc = eventComesFromRpc](auto const& value) mutable {
                          if (!*eventComesFromRpc)
                              hub.call("observed_" + std::string(syncId(ObservedT{})), nlohmann::json(value));
                      }),
              }
        {}

      public:
        explicit Synchronizer(RpcHub& hub, ObservedT& observed)
            : Synchronizer{hub, observed, std::make_shared<bool>(false)}
        {}
#endif
    };

    template <typename SharedObservedT>
    requires SharedSynchronized<SharedObservedT>
    class Synchronizer<SharedObservedT> : public SynchronizerBase<SharedObservedT>
    {
      public:
        using ObservedT = typename SharedObservedT::value_type;

#ifdef NUI_FRONTEND

      private:
        explicit Synchronizer(SharedObservedT const& observed, std::shared_ptr<bool> const& eventComesFromRpc)
            : SynchronizerBase<SharedObservedT>{
                  RpcClient::autoRegisterFunction(
                      "observed_" + std::string(syncId(observed)),
                      [weak = std::weak_ptr{observed}, eventComesFromRpc = eventComesFromRpc](Nui::val value) mutable {
                          auto observed = weak.lock();
                          if (!observed)
                              return;

                          {
                              *eventComesFromRpc = true;
                              convertFromVal(value, observed->modifyNow().value());
                          }
                          *eventComesFromRpc = false;
                      }),
                  Nui::smartListen(
                      observed,
                      [eventComesFromRpc = eventComesFromRpc](auto const& value) mutable {
                          if (!*eventComesFromRpc)
                              RpcClient::call("observed_" + std::string(syncId(ObservedT{})), convertToVal(value));
                      }),
              }
        {}

      public:
        explicit Synchronizer(SharedObservedT& observed)
            : Synchronizer{observed, std::make_shared<bool>(false)}
        {}
#else

      private:
        explicit Synchronizer(
            RpcHub& hub,
            SharedObservedT const& observed,
            std::shared_ptr<bool> const& eventComesFromRpc)
            : SynchronizerBase<SharedObservedT>{
                  hub.autoRegisterFunction(
                      "observed_" + std::string(syncId(observed)),
                      [weak = std::weak_ptr{observed},
                       eventComesFromRpc = eventComesFromRpc](nlohmann::json const& value) mutable {
                          auto observed = weak.lock();
                          if (!observed)
                              return;

                          {
                              *eventComesFromRpc = true;
                              *observed = value.template get<typename SharedObservedT::value_type::observed_type>();
                              observed->eventContext().sync();
                          }
                          *eventComesFromRpc = false;
                      }),
                  Nui::smartListen(
                      observed,
                      [&hub, eventComesFromRpc = eventComesFromRpc](auto const& value) mutable {
                          if (!*eventComesFromRpc)
                              hub.call("observed_" + std::string(syncId(ObservedT{})), nlohmann::json(value));
                      }),
              }
        {}

      public:
        explicit Synchronizer(RpcHub& hub, SharedObservedT const& observed)
            : Synchronizer{hub, observed, std::make_shared<bool>(false)}
        {}
#endif
    };
}