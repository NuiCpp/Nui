#pragma once

#include <emscripten/val.h>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/functions.hpp>
#include <nui/utility/meta/function_traits.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <string>
#include <cstdint>
#include <tuple>

namespace Nui
{
    namespace Detail
    {
        template <typename ReturnType, typename ArgsTypes, typename IndexSeq>
        struct FunctionWrapperImpl
        {};

        template <typename ArgT>
        constexpr static auto extractMember(emscripten::val const& val) -> decltype(auto)
        {
            if constexpr (std::is_same_v<std::decay_t<ArgT>, emscripten::val>)
                return val;
            else
            {
                std::decay_t<ArgT> value;
                convertFromVal(val, value);
                return value;
            }
        }

        template <typename ReturnType>
        struct FunctionWrapperImpl<ReturnType, std::tuple<emscripten::val>, std::index_sequence<0>>
        {
            template <typename FunctionT>
            constexpr static auto wrapFunction(FunctionT&& func)
            {
                return [func = std::move(func)](emscripten::val const& args) mutable {
                    func(args);
                };
            }
        };

        template <typename ReturnType, typename ArgType>
        struct FunctionWrapperImpl<ReturnType, std::tuple<ArgType>, std::index_sequence<0>>
        {
            template <typename FunctionT>
            constexpr static auto wrapFunction(FunctionT&& func)
            {
                return [func = std::move(func)](emscripten::val const& arg) mutable {
                    func(extractMember<ArgType>(arg));
                };
            }
        };

        template <typename ReturnType, typename... ArgsTypes, std::size_t... Is>
        struct FunctionWrapperImpl<ReturnType, std::tuple<ArgsTypes...>, std::index_sequence<Is...>>
        {
            template <typename FunctionT>
            constexpr static auto wrapFunction(FunctionT&& func)
            {
                return [func = std::move(func)](emscripten::val const& args) mutable {
                    func(extractMember<ArgsTypes>(args[Is])...);
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

    class RpcClient
    {
      public:
        class RemoteCallable
        {
          public:
            template <typename... Args>
            auto operator()(Args&&... args) const
            {
                using namespace std::string_literals;
                if (!resolve())
                {
                    Console::error("Remote callable with name '"s + name_ + "' is undefined");
                    return emscripten::val::undefined();
                }
                if (backChannel_.empty())
                    return callable_(convertToVal(args)...);
                else
                    return callable_(convertToVal(backChannel_), convertToVal(args)...);
            }
            auto operator()(emscripten::val val) const
            {
                using namespace std::string_literals;
                if (!resolve())
                {
                    Console::error("Remote callable with name '"s + name_ + "' is undefined");
                    return emscripten::val::undefined();
                }
                if (backChannel_.empty())
                    return callable_(val);
                else
                    return callable_(convertToVal(backChannel_), val);
            }

            RemoteCallable(std::string name)
                : name_{std::move(name)}
                , backChannel_{}
                , callable_{emscripten::val::undefined()}
                , isSet_{false}
            {}

            RemoteCallable(std::string name, std::string backChannel)
                : name_{std::move(name)}
                , backChannel_{std::move(backChannel)}
                , callable_{emscripten::val::undefined()}
                , isSet_{false}
            {}

          private:
            bool resolve() const
            {
                using namespace std::string_literals;
                if (isSet_)
                    return true;

                const auto rpcObject = emscripten::val::global("nui_rpc");
                if (rpcObject.isUndefined())
                    return false;

                callable_ = emscripten::val::global("nui_rpc")["backend"][name_.c_str()];
                isSet_ = !callable_.isUndefined();
                return isSet_;
            }

          private:
            std::string name_;
            std::string backChannel_;
            mutable emscripten::val callable_;
            mutable bool isSet_;
        };

        /**
         * @brief Get a callable remote function.
         *
         * @param name Name of the function.
         * @return auto A wrapper that can be called.
         */
        static auto getRemoteCallable(std::string name)
        {
            return RemoteCallable{std::move(name)};
        }

        /**
         * @brief Get a callable remote function and register a temporary callable for a response.
         */
        template <typename FunctionT>
        static auto getRemoteCallableWithBackChannel(std::string name, FunctionT&& func)
        {
            return RemoteCallable{std::move(name), registerFunctionOnce(std::forward<FunctionT>(func))};
        }

        /**
         * @brief Registers a single shot function that is removed after it was called.
         *
         * @param func The function to call.
         * @return std::string The generated name of the function.
         */
        template <typename FunctionT>
        static std::string registerFunctionOnce(FunctionT&& func)
        {
            using namespace std::string_literals;
            if (emscripten::val::global("nui_rpc").isUndefined())
            {
                Console::error("rpc was not setup by backend"s);
                return {};
            }
            auto tempId = emscripten::val::global("nui_rpc")["tempId"].as<uint32_t>() + 1;
            emscripten::val::global("nui_rpc").set("tempId", tempId);
            const auto tempIdString = "temp_"s + std::to_string(tempId);
            // TODO: dry?
            emscripten::val::global("nui_rpc")["frontend"].set(
                tempIdString,
                Nui::bind(
                    [func = Detail::FunctionWrapper<FunctionT>::wrapFunction(std::forward<FunctionT>(func)),
                     tempIdString](emscripten::val param) mutable {
                        func(param);
                        emscripten::val::global("nui_rpc")["frontend"].delete_(tempIdString);
                    },
                    std::placeholders::_1));
            return tempIdString;
        }

        /**
         * @brief Register a permanent function that is callable from the backend.
         *
         * @param name The name of the function.
         * @param func The function itself.
         */
        template <typename FunctionT>
        static void registerFunction(std::string const& name, FunctionT&& func)
        {
            using namespace std::string_literals;
            if (emscripten::val::global("nui_rpc").isUndefined())
            {
                Console::error("rpc was not setup by backend"s);
                return;
            }
            emscripten::val::global("nui_rpc")["frontend"].set(
                name.c_str(),
                Nui::bind(
                    [func = Detail::FunctionWrapper<FunctionT>::wrapFunction(std::forward<FunctionT>(func))](
                        emscripten::val param) mutable {
                        func(param);
                    },
                    std::placeholders::_1));
        }

        static void unregisterFunction(std::string const& name)
        {
            using namespace std::string_literals;
            if (emscripten::val::global("nui_rpc").isUndefined())
            {
                Console::error("rpc was not setup by backend"s);
                return;
            }
            emscripten::val::global("nui_rpc")["frontend"].delete_(name.c_str());
        }
    };
}