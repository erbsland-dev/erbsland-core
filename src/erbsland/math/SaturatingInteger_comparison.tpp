// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerConversion.hpp"
#include "IntegerTraits.hpp"

namespace erbsland::math {

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::operator==(T other) const noexcept -> bool {
    return mixedIntegerCompare(_value, convertToNativeInt(other)) == std::strong_ordering::equal;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::operator!=(T other) const noexcept -> bool {
    return mixedIntegerCompare(_value, convertToNativeInt(other)) != std::strong_ordering::equal;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::operator<(T other) const noexcept -> bool {
    return mixedIntegerCompare(_value, convertToNativeInt(other)) == std::strong_ordering::less;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::operator<=(T other) const noexcept -> bool {
    return mixedIntegerCompare(_value, convertToNativeInt(other)) != std::strong_ordering::greater;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::operator>(T other) const noexcept -> bool {
    return mixedIntegerCompare(_value, convertToNativeInt(other)) == std::strong_ordering::greater;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::operator>=(T other) const noexcept -> bool {
    return mixedIntegerCompare(_value, convertToNativeInt(other)) != std::strong_ordering::less;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::compare(T value) const noexcept -> std::strong_ordering {
    return mixedIntegerCompare(_value, convertToNativeInt(value));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::convertToNativeInt(T value) noexcept -> NativeIntegerOfT<T> {
    return toNativeInteger(value);
}

}
