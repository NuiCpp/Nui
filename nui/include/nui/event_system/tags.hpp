#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/tags_traits/sync_id.hpp>

namespace Nui
{
    template <typename... T>
    struct TagContainer : public T...
    {};

    template <typename TagContainer, typename = void>
    struct IsSynchronizedImpl : std::false_type
    {};

    template <typename TagContainer>
    struct IsSynchronizedImpl<TagContainer, std::void_t<decltype(TagContainer::syncId)>> : std::true_type
    {};

    template <typename TagContainer>
    concept IsSynchronized = IsSynchronizedImpl<TagContainer>::value;

    template <typename ObservedT>
    struct SynchronizedImpl : std::false_type
    {};

    template <typename T, typename... Tags>
    struct SynchronizedImpl<Observed<T, TagContainer<Tags...>>> : IsSynchronizedImpl<TagContainer<Tags...>>
    {};

    template <typename T>
    concept Synchronized = SynchronizedImpl<T>::value;

    template <typename T>
    concept SharedSynchronized = IsSharedObserved<T> && Synchronized<typename T::element_type>;
}