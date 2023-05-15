#pragma once

#include <pre/type_traits/function_traits.hpp>
#include <pre/functional/to_std_function.hpp>

#include <nui/utility/meta/function_traits.hpp>

#include <functional>
#include <any>
#include <optional>
#include <concepts>

namespace Nui::Tests::Engine
{
    namespace Detail
    {
        struct FunctionYes
        {};
        struct FunctionNo
        {};

        template <typename Functor, typename ReturnT, typename... Rest>
        FunctionYes FunctionInferenceHelper(ReturnT (Functor::*)(Rest...));
        template <typename Functor, typename ReturnT, typename... Rest>
        FunctionYes FunctionInferenceHelper(ReturnT (Functor::*)(Rest...) const);
        template <typename ReturnT, typename... Rest>
        FunctionYes FunctionInferenceHelper(ReturnT (*)(Rest...));

        // noexcept
        template <typename Functor, typename ReturnT, typename... Rest>
        FunctionYes FunctionInferenceHelper(ReturnT (Functor::*)(Rest...) noexcept);
        template <typename Functor, typename ReturnT, typename... Rest>
        FunctionYes FunctionInferenceHelper(ReturnT (Functor::*)(Rest...) const noexcept);
        template <typename ReturnT, typename... Rest>
        FunctionYes FunctionInferenceHelper(ReturnT (*)(Rest...) noexcept);

        template <typename T>
        FunctionNo FunctionInferenceHelper(T);

        template <typename FunctionT, typename Enable = void>
        struct FunctionTypesImpl
        {
            using deduced = FunctionNo;
        };
        template <typename FunctionT>
        struct FunctionTypesImpl<FunctionT, std::void_t<decltype(FunctionInferenceHelper(&FunctionT::operator()))>>
        {
            using deduced = FunctionYes;
        };
        template <typename FunctorT, typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (FunctorT::*)(Arguments...), void>
        {
            using deduced = FunctionYes;
        };
        template <typename FunctorT, typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (FunctorT::*)(Arguments...) const, void>
        {
            using deduced = FunctionYes;
        };
        template <typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (*)(Arguments...), void>
        {
            using deduced = FunctionYes;
        };

        template <typename ArgTuple>
        struct FunctionSignature
        {};

        template <typename... Args>
        struct FunctionSignature<std::tuple<Args...>>
        {
            using type = std::function<Value(Args...)>;
        };

        template <typename T>
        using FunctionSignature_t = typename FunctionSignature<T>::type;
    }

    template <typename T>
    concept Callable = requires(T t) {
        {
            typename Detail::FunctionTypesImpl<T>::deduced{}
        } -> std::same_as<Detail::FunctionYes>;
    };

    class Function
    {
      public:
        Function() = default;
        Function(const Function&) = default;
        Function(Function&&) = default;
        Function& operator=(const Function&) = default;
        Function& operator=(Function&&) = default;

        template <typename T>
        requires Callable<T>
        Function(T&& function)
            : callable_{Detail::FunctionSignature_t<FunctionArgumentTypes_t<T>>{
                  [function = std::forward<T>(function)](auto&&... args) {
                      if constexpr (std::is_same_v<FunctionReturnType_t<T>, void>)
                          return function(std::forward<decltype(args)>(args)...), Value{};
                      else
                          return Value{function(std::forward<decltype(args)>(args)...)};
                  }}}
        {}

        template <typename... Args>
        Function(std::function<Value(Args const&...)> handler)
            : callable_{handler}
        {}

        template <typename... Args>
        Value operator()(Args&&... args) const
        {
            return std::any_cast<std::function<Value(Args...)>>(callable_)(std::forward<Args>(args)...);
        }

      private:
        std::any callable_;
    };
}