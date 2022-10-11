#pragma once

#include <emscripten/val.h>

#include <nui/frontend/api/console.hpp>

#include <string>

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
                return callable_(emscripten::val{args}...);
            }

            RemoteCallable(std::string name)
                : name_{std::move(name)}
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
            mutable emscripten::val callable_;
            mutable bool isSet_;
        };

        static auto getRemoteCallable(std::string name)
        {
            using namespace std::string_literals;
            return RemoteCallable{std::move(name)};
        }

        template <typename FunctionT>
        static void registerFunction(std::string const& name, FunctionT&& func)
        {
            using namespace std::string_literals;
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