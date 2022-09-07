#pragma once

#include <execution>
#include <type_traits>

namespace transport_catalogue::detail /* template helpers */ {
    template <bool Condition>
    using EnableIf = typename std::enable_if_t<Condition, bool>;

    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsConvertibleV = std::is_convertible<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsSameV = std::is_same<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using IsBaseOf = std::is_base_of<BaseType, std::decay_t<DerivedType>>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsBaseOfV = IsBaseOf<FromType, ToType>::value;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;

    template <class ExecutionPolicy>
    using IsExecutionPolicy = std::is_execution_policy<std::decay_t<ExecutionPolicy>>;

    template <class ExecutionPolicy>
    using EnableForExecutionPolicy = typename std::enable_if_t<IsExecutionPolicy<ExecutionPolicy>::value, bool>;

    template <typename Iterator>
    using IterValueType = typename std::iterator_traits<Iterator>::value_type;

    template<typename T>
    using RemoveRef = std::remove_reference_t<T>;
}