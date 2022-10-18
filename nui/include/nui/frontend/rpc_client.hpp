#pragma once

#include <emscripten/val.h>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/functions.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <string>
#include <cstdint>

namespace Nui
{
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
                if (rpcObject.typeOf().as<std::string>() == "undefined")
                    return false;

                callable_ = emscripten::val::global("nui_rpc")["backend"][name_.c_str()];
                isSet_ = callable_.typeOf().as<std::string>() != "undefined";
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
            if (emscripten::val::global("nui_rpc").typeOf().as<std::string>() == "undefined")
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
                    [func = std::move(func), tempIdString](emscripten::val param) {
                        func(param);
                        emscripten::val::global("nui_rpc")["frontend"].set(tempIdString, emscripten::val::undefined());
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
            if (emscripten::val::global("nui_rpc").typeOf().as<std::string>() == "undefined")
            {
                Console::error("rpc was not setup by backend"s);
                return;
            }
            emscripten::val::global("nui_rpc")["frontend"].set(
                (name).c_str(),
                Nui::bind(
                    [func = std::move(func)](emscripten::val param) {
                        func(param);
                    },
                    std::placeholders::_1));
        }
    };
}