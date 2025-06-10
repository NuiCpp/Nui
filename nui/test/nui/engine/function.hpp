#pragma once

#include <traits/functions.hpp>

#include <boost/type_index.hpp>

#include <functional>
#include <any>
#include <concepts>
#include <iostream>

namespace emscripten
{
    class val;
}

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
            using type = std::function<emscripten::val(Args...)>;
        };

        template <typename T>
        using FunctionSignature_t = typename FunctionSignature<T>::type;

        template <typename T>
        struct TupleTypePrint
        {};

        template <typename... Args>
        struct TupleTypePrint<std::tuple<Args...>>
        {
            static std::string toString()
            {
                std::string types;
                ((types += boost::typeindex::type_id<Args>().pretty_name() += ", "), ...);
                types.pop_back();
                types.pop_back();
                return types;
            }
        };
    }

    template <typename T>
    concept Callable = requires(T t) {
        { typename Detail::FunctionTypesImpl<T>::deduced{} } -> std::same_as<Detail::FunctionYes>;
    };

    class Function
    {
      public:
        Function() = default;
        Function(const Function&) = default;
        Function(Function&&) = default;
        Function& operator=(const Function&) = default;
        Function& operator=(Function&&) = default;
        ~Function() = default;

        template <typename T>
        requires Callable<T>
        Function(T&& function);

        template <typename... Args>
        Function(std::function<emscripten::val(Args const&...)> handler);

        template <typename... Args>
        emscripten::val operator()(Args&&... args) const;

        void print(int indent) const
        {
            for (int i = 0; i < indent; ++i)
                std::cout << "  ";
            std::cout << signature_;
        }

      private:
        std::any callable_;
        std::string signature_;
    };
}