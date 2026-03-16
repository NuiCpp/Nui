#include <type_traits>
#include <memory>

namespace Nui
{
    template <typename>
    struct IsSharedPtrImpl : std::false_type
    {};
    template <typename T>
    struct IsSharedPtrImpl<std::shared_ptr<T>> : std::true_type
    {};

    template <typename T>
    concept IsSharedPtr = IsSharedPtrImpl<std::decay_t<T>>::value;
}