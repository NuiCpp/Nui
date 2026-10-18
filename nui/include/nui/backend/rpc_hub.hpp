#pragma once

#include <nui/window.hpp>
#include <nui/data_structures/selectables_registry.hpp>
#include <nui/utility/meta/pick_first.hpp>
#include <nui/shared/on_destroy.hpp>

#include <traits/functions.hpp>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace Nui
{
    namespace Detail
    {
        template <typename ReturnType, typename ArgsTypes, typename IndexSeq>
        struct FunctionWrapperImpl
        {};

        /**
         * @brief Unpack a single RPC parameter from its nlohmann JSON form.
         *
         *        The frontend wraps 64-bit integers into `{_u64_hi, _u64_lo}`
         *        objects (see convertToVal in val_conversion.hpp) because JS
         *        JSON.stringify can't serialize BigInt. Any handler whose
         *        signature takes a bare `uint64_t`/`int64_t` parameter would
         *        otherwise fail the default `json.get<uint64_t>()` type check
         *        — this overload rebuilds the scalar from the split shape,
         *        and still accepts legacy plain-number payloads for
         *        backward compatibility.
         */
        template <typename ArgT>
        constexpr static auto extractJsonMember(nlohmann::json const& json) -> decltype(auto)
        {
            using Decayed = std::decay_t<ArgT>;
            if constexpr (std::is_same_v<Decayed, nlohmann::json>)
            {
                return json;
            }
            else if constexpr (std::is_same_v<Decayed, std::uint64_t> || std::is_same_v<Decayed, std::int64_t>)
            {
                constexpr unsigned u32BitCount = 32;
                if (json.is_object() && json.contains("_u64_hi") && json.contains("_u64_lo"))
                {
                    const auto hi = static_cast<std::uint64_t>(json.at("_u64_hi").get<std::uint32_t>());
                    const auto lo = static_cast<std::uint64_t>(json.at("_u64_lo").get<std::uint32_t>());
                    const std::uint64_t combined = (hi << u32BitCount) | lo;
                    if constexpr (std::is_same_v<Decayed, std::int64_t>)
                        return static_cast<std::int64_t>(combined);
                    else
                        return combined;
                }
                return json.get<Decayed>();
            }
            else
            {
                return json.get<Decayed>();
            }
        }

        template <typename ReturnType>
        struct FunctionWrapperImpl<ReturnType, std::tuple<nlohmann::json>, std::index_sequence<0>>
        {
            template <typename FunctionT>
            constexpr static auto wrapFunction(FunctionT&& func)
            {
                return [func = std::forward<FunctionT>(func)](nlohmann::json const& args) mutable {
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
                return [func = std::forward<FunctionT>(func)](nlohmann::json const& args) mutable {
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
                  typename Traits::FunctionTraits<std::decay_t<FunctionT>>::ReturnType,
                  typename Traits::FunctionTraits<std::decay_t<FunctionT>>::ArgsTuple>
        {};
    }

    class RpcHub
    {
      public:
        explicit RpcHub(Window& window);
        ~RpcHub() = default;
        RpcHub(const RpcHub&) = delete;
        RpcHub& operator=(const RpcHub&) = delete;
        RpcHub(RpcHub&&) = delete;
        RpcHub& operator=(RpcHub&&) = delete;

        constexpr static char const* remoteCallScript = R"(
            (function() {{
                if (globalThis.nui_rpc.frontend.hasOwnProperty("{0}")) {{
                    globalThis.nui_rpc.frontend["{0}"]({1});
                    return;
                }}

                globalThis.nui_rpc.errors = globalThis.nui_rpc.errors || [];
                globalThis.nui_rpc.errors.push("Function {0} does not exist.");
                if (globalThis.nui_rpc.errors.length > 100) {{
                    globalThis.nui_rpc.errors.shift();
                }}
            }})();
        )";

        struct AutoUnregister : public OnDestroy
        {
            AutoUnregister(RpcHub const* hub, std::string name)
                : OnDestroy{[hub, name = std::move(name)]() {
                    hub->unregisterFunction(name);
                }}
            {}
            ~AutoUnregister() = default;

            AutoUnregister(AutoUnregister const&) = delete;
            AutoUnregister(AutoUnregister&& other) noexcept
                : OnDestroy{std::move(other)}
            {}
            AutoUnregister& operator=(AutoUnregister const&) = delete;
            AutoUnregister& operator=(AutoUnregister&& other) noexcept
            {
                OnDestroy::operator=(std::move(other));
                return *this;
            }

            void reset()
            {
                trigger();
            }
        };

        template <typename T>
        AutoUnregister autoRegisterFunction(std::string const& name, T&& func) const
        {
            registerFunction(name, std::forward<T>(func));
            return AutoUnregister{this, name};
        }

        template <typename T>
        void registerFunction(std::string const& name, T&& func) const
        {
            // window is threadsafe
            window_->bind(name, Detail::FunctionWrapper<T>::wrapFunction(std::forward<T>(func)));
        }
        void unregisterFunction(std::string const& name) const
        {
            // window is threadsafe
            window_->unbind(name);
        }

        /**
         * @brief Returns the attached window.
         *
         * @return Window&
         */
        Window& window() const
        {
            return *window_;
        }

      private:
        /**
         * @brief Normalize a single outgoing RPC argument so 64-bit integers
         *        ship in the `{_u64_hi, _u64_lo}` split form — identical to
         *        what struct members and frontend-originated payloads use.
         *
         *        Bare `uint64_t`/`int64_t` args otherwise fall into nlohmann's
         *        default serializer which emits a plain JSON number; that's
         *        lossy for values above JS's 53-bit safe integer range and
         *        asymmetric with the split form the frontend sends back.
         *        Non-integer args are passed through untouched so struct
         *        describe-based serialization (which already handles u64
         *        members via shared_data's to_json_impl) still kicks in.
         */
        template <typename Arg>
        static auto normalizeCallRemoteArg(Arg&& arg)
        {
            using Decayed = std::decay_t<Arg>;
            if constexpr (std::is_same_v<Decayed, std::uint64_t> || std::is_same_v<Decayed, std::int64_t>)
            {
                constexpr unsigned u32BitCount = 32;
                constexpr std::uint64_t u32Mask = 0xFFFFFFFFu;
                const auto uv = static_cast<std::uint64_t>(arg);
                return nlohmann::json{
                    {"_u64_hi", static_cast<std::uint32_t>((uv >> u32BitCount) & u32Mask)},
                    {"_u64_lo", static_cast<std::uint32_t>(uv & u32Mask)},
                };
            }
            else
            {
                return std::forward<Arg>(arg);
            }
        }

      public:
        /**
         * @brief For technical reasons these cannot return a value currently.
         *
         * @tparam Args
         * @param name
         * @param args
         */
        template <typename... Args>
        void callRemote(std::string const& name, Args&&... args) const
        {
            callRemoteImpl(name, nlohmann::json{normalizeCallRemoteArg(std::forward<Args>(args))...});
        }
        template <typename Arg>
        void callRemote(std::string const& name, Arg&& arg) const
        {
            nlohmann::json json = normalizeCallRemoteArg(std::forward<Arg>(arg));
            callRemoteImpl(name, json);
        }
        void callRemote(std::string const& name, nlohmann::json const& json) const
        {
            callRemoteImpl(name, json);
        }
        // I dont want to remove this overload in case there are some rvalue nlohmanns?
        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        void callRemote(std::string const& name, nlohmann::json&& json) const
        {
            callRemoteImpl(name, json);
        }
        void callRemote(std::string const& name) const
        {
            callRemoteImpl(name);
        }

        // alias for callRemote
        template <typename... Args>
        void call(std::string const& name, Args&&... args) const
        {
            callRemote(name, std::forward<Args>(args)...);
        }

        /**
         * @brief Enables file dialog functionality
         */
        void enableFileDialogs() const;

        /**
         * @brief Enables file class in the frontend.
         */
        void enableFile();

        /**
         * @brief Enables opening of the devTools, terminating the window from the view...
         */
        void enableWindowFunctions() const;

        /**
         * @brief Enables fetch functionality.
         */
        void enableFetch() const;

        /**
         * @brief Enables the throttle functionality.
         */
        void enableThrottle();

        /**
         * @brief Enables the setInterval and setTimeout functionality.
         */
        void enableTimer();

        /**
         * @brief Enables the screen functionality.
         */
        void enableScreen();

        /**
         * @brief Enables the environment variable functionality.
         */
        void enableEnvironmentVariables();

        /**
         * @brief Enables all functionality.
         */
        void enableAll();

        template <typename ManagerT>
        void* accessStateStore(std::string const& id)
        {
            std::scoped_lock lock{guard_};
            auto iter = stateStores_.find(id);
            if (iter == stateStores_.end())
            {
                return stateStores_
                    .insert(
                        {id,
                         std::unique_ptr<void, std::function<void(void*)>>{
                             ManagerT::create(),
                             [](void* ptr) {
                                 ManagerT::destroy(ptr);
                             }}})
                    .first->second.get();
            }
            return iter->second.get();
        }

        void markRpcAsInitialized();

      private:
        void callRemoteImpl(std::string const& name, nlohmann::json const& json) const
        {
            // window is threadsafe.
            window_->callFrontend(name, json.dump());
        }
        void callRemoteImpl(std::string const& name) const
        {
            // window is threadsafe.
            window_->callFrontend(name, "");
        }

      private:
        std::recursive_mutex guard_;
        Window* window_;
        std::unordered_map<std::string, std::unique_ptr<void, std::function<void(void*)>>> stateStores_;
    };
}