#include <type_traits>
#include <functional>

namespace Nui
{
    template <typename>
    struct IsStdFunctionImpl : std::false_type
    {};
    template <typename RetT, typename... ArgsT>
    struct IsStdFunctionImpl<std::function<RetT(ArgsT...)>> : std::true_type
    {};

    template <typename T>
    concept IsStdFunction = IsStdFunctionImpl<std::decay_t<T>>::value;
}