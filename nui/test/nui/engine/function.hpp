#pragma once

#include <functional>
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
            : function_{}
        {}

        // TODO:
        // template <typename... Args>
        // auto operator()(Args&&... args) const
        // {
        //     return function_(std::forward<Args>(args)...);
        // }

      private:
        // TODO:
        std::function<Value(Value const&)> function_;
    };
}