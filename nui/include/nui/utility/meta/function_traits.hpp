#pragma once

namespace Nui
{
    namespace Detail
    {
        // Helper for obtaining the argument types of a function
        template <typename Functor, typename ReturnT, typename... Rest>
        std::tuple<Rest...> FunctionInferenceHelper(ReturnT (Functor::*)(Rest...));
        template <typename Functor, typename ReturnT, typename... Rest>
        std::tuple<Rest...> FunctionInferenceHelper(ReturnT (Functor::*)(Rest...) const);
        template <typename ReturnT, typename... Rest>
        std::tuple<Rest...> FunctionInferenceHelper(ReturnT (*)(Rest...));

        template <typename Functor, typename ReturnT, typename... Rest>
        ReturnT FunctionInferenceHelperRet(ReturnT (Functor::*)(Rest...));
        template <typename Functor, typename ReturnT, typename... Rest>
        ReturnT FunctionInferenceHelperRet(ReturnT (Functor::*)(Rest...) const);
        template <typename ReturnT, typename... Rest>
        ReturnT FunctionInferenceHelperRet(ReturnT (*)(Rest...));

        // noexcept
        template <typename Functor, typename ReturnT, typename... Rest>
        std::tuple<Rest...> FunctionInferenceHelper(ReturnT (Functor::*)(Rest...) noexcept);
        template <typename Functor, typename ReturnT, typename... Rest>
        std::tuple<Rest...> FunctionInferenceHelper(ReturnT (Functor::*)(Rest...) const noexcept);
        template <typename ReturnT, typename... Rest>
        std::tuple<Rest...> FunctionInferenceHelper(ReturnT (*)(Rest...) noexcept);

        template <typename Functor, typename ReturnT, typename... Rest>
        ReturnT FunctionInferenceHelperRet(ReturnT (Functor::*)(Rest...) noexcept);
        template <typename Functor, typename ReturnT, typename... Rest>
        ReturnT FunctionInferenceHelperRet(ReturnT (Functor::*)(Rest...) const noexcept);
        template <typename ReturnT, typename... Rest>
        ReturnT FunctionInferenceHelperRet(ReturnT (*)(Rest...) noexcept);

        // Allows to get the argument types of a function.
        template <typename FunctionT>
        struct FunctionTypesImpl
        {
            using type = decltype(FunctionInferenceHelper(&FunctionT::operator()));
            using return_type = decltype(FunctionInferenceHelperRet(&FunctionT::operator()));
        };
        template <typename FunctorT, typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (FunctorT::*)(Arguments...)>
        {
            using return_type = ReturnT;
            using type = std::tuple<Arguments...>;
        };
        template <typename FunctorT, typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (FunctorT::*)(Arguments...) const>
        {
            using return_type = ReturnT;
            using type = std::tuple<Arguments...>;
        };
        template <typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (*)(Arguments...)>
        {
            using return_type = ReturnT;
            using type = std::tuple<Arguments...>;
        };

        // noexcept
        template <typename FunctorT, typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (FunctorT::*)(Arguments...) noexcept>
        {
            using return_type = ReturnT;
            using type = std::tuple<Arguments...>;
        };
        template <typename FunctorT, typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (FunctorT::*)(Arguments...) const noexcept>
        {
            using return_type = ReturnT;
            using type = std::tuple<Arguments...>;
        };
        template <typename ReturnT, typename... Arguments>
        struct FunctionTypesImpl<ReturnT (*)(Arguments...) noexcept>
        {
            using return_type = ReturnT;
            using type = std::tuple<Arguments...>;
        };
    }

    template <typename FunctionT>
    struct FunctionArgumentTypes
    {
        using type = typename Detail::FunctionTypesImpl<std::decay_t<FunctionT>>::type;
    };

    template <typename FunctionT>
    struct FunctionReturnType
    {
        using type = typename Detail::FunctionTypesImpl<std::decay_t<FunctionT>>::return_type;
    };

    template <typename T>
    using FunctionArgumentTypes_t = typename FunctionArgumentTypes<T>::type;
    template <typename T>
    using FunctionReturnType_t = typename FunctionReturnType<T>::type;
}