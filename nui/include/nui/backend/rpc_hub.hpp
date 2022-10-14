#pragma once

#include <nui/window.hpp>
#include <nui/data_structures/selectables_registry.hpp>
#include <nui/utility/meta/function_traits.hpp>
#include <nui/utility/meta/pick_first.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <thread>

namespace Nui
{
    namespace Detail
    {
        template <typename ReturnType, typename ArgsTypes, typename IndexSeq>
        struct FunctionWrapperImpl
        {};

        template <typename ArgT>
        constexpr static auto extractJsonMember(nlohmann::json const& json) -> decltype(auto)
        {
            if constexpr (std::is_same_v<std::decay_t<ArgT>, nlohmann::json>)
                return json;
            else
                return json.get<std::decay_t<ArgT>>();
        }

        template <typename ReturnType>
        struct FunctionWrapperImpl<ReturnType, std::tuple<nlohmann::json>, std::index_sequence<0>>
        {
            template <typename FunctionT>
            constexpr static auto wrapFunction(FunctionT&& func)
            {
                return [func = std::move(func)](nlohmann::json const& args) {
                    func(args);
                };
            }
        };

        template <typename ReturnType, typename... ArgsTypes, std::size_t... Is>
        struct FunctionWrapperImpl<ReturnType, std::tuple<ArgsTypes...>, std::index_sequence<Is...>>
        {
            template <typename FunctionT>
            constexpr static auto wrapFunction(FunctionT&& func)
            {
                return [func = std::move(func)](nlohmann::json const& args) {
                    func(extractJsonMember<ArgsTypes>(args[Is])...);
                };
            }
        };

        template <typename ReturnType, typename ArgsTypes>
        struct FunctionWrapperImpl2
        {};

        template <typename ReturnType, typename... ArgsTypes>
        struct FunctionWrapperImpl2<ReturnType, std::tuple<ArgsTypes...>>
            : public FunctionWrapperImpl<ReturnType, std::tuple<ArgsTypes...>, std::index_sequence_for<ArgsTypes...>>
        {};

        template <typename FunctionT>
        struct FunctionWrapper
            : public FunctionWrapperImpl2<
                  FunctionReturnType_t<std::decay_t<FunctionT>>,
                  FunctionArgumentTypes_t<std::decay_t<FunctionT>>>
        {};
    }

    class RpcHub
    {
      public:
        RpcHub(Window& window);
        ~RpcHub() = default;
        RpcHub(const RpcHub&) = delete;
        RpcHub& operator=(const RpcHub&) = delete;
        RpcHub(RpcHub&&) = default;
        RpcHub& operator=(RpcHub&&) = default;

        constexpr static char const* remoteCallScript = R"(
            (function() {{ 
                globalThis.nui_rpc.frontend["{}"]({});
            }})();
        )";

        template <typename T>
        void registerFunction(std::string const& name, T&& func)
        {
            using namespace std::string_literals;
            window_->bind(name, Detail::FunctionWrapper<T>::wrapFunction(std::forward<T>(func)));
        }

        /**
         * @brief For technical reasons these cannot return a value currently.
         *
         * @tparam Args
         * @param name
         * @param args
         */
        template <typename... Args>
        void callRemote(std::string const& name, Args&&... args)
        {
            callRemoteImpl(name, nlohmann::json{std::forward<Args>(args)...});
        }
        void callRemote(std::string const& name, nlohmann::json const& json)
        {
            callRemoteImpl(name, json);
        }
        void callRemote(std::string const& name, nlohmann::json&& json)
        {
            callRemoteImpl(name, json);
        }

        void enableFileDialogs();

      private:
        void callRemoteImpl(std::string const& name, nlohmann::json const& json)
        {
            using namespace std::string_literals;
            window_->eval(fmt::format(remoteCallScript, name, json.dump()));
        }

      private:
        Window* window_;
    };
}