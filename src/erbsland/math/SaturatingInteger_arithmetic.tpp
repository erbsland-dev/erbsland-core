// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

namespace erbsland::math {

template <NativeInteger tValue>
template <AnyIntegerType T>
    requires SignCompatibleIntegerOperand<tValue, T>
auto SaturatingInteger<tValue>::operator+(T other) const noexcept
    -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>> {
    using C = CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>;
    return SaturatingInteger<C>{saturatingAdd(static_cast<C>(_value), static_cast<C>(convertToNativeInt(other)))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::operator+=(T other) noexcept -> SaturatingInteger & {
    _value = saturatingAdd(_value, convertToNativeInt(other));
    return *this;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
    requires SignCompatibleIntegerOperand<tValue, T>
auto SaturatingInteger<tValue>::operator-(T other) const noexcept
    -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>> {
    using C = CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>;
    return SaturatingInteger<C>{saturatingSubtract(static_cast<C>(_value), static_cast<C>(convertToNativeInt(other)))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::operator-=(T other) noexcept -> SaturatingInteger & {
    _value = saturatingSubtract(_value, convertToNativeInt(other));
    return *this;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
    requires SignCompatibleIntegerOperand<tValue, T>
auto SaturatingInteger<tValue>::operator*(T other) const noexcept
    -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>> {
    using C = CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>;
    return SaturatingInteger<C>{saturatingMultiply(static_cast<C>(_value), static_cast<C>(convertToNativeInt(other)))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::operator*=(T other) noexcept -> SaturatingInteger & {
    _value = saturatingMultiply(_value, convertToNativeInt(other));
    return *this;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
    requires SignCompatibleIntegerOperand<tValue, T>
auto SaturatingInteger<tValue>::operator/(T other) const noexcept
    -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>> {
    using C = CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>;
    return SaturatingInteger<C>{saturatingDivide(static_cast<C>(_value), static_cast<C>(convertToNativeInt(other)))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::operator/=(T other) noexcept -> SaturatingInteger & {
    _value = saturatingDivide(_value, convertToNativeInt(other));
    return *this;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
    requires SignCompatibleIntegerOperand<tValue, T>
auto SaturatingInteger<tValue>::operator%(T other) const noexcept
    -> SaturatingInteger<CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>> {
    using C = CompatibleNativeIntegerT<NativeValue, NativeIntegerOfT<T>>;
    return SaturatingInteger<C>{saturatingModulo(static_cast<C>(_value), static_cast<C>(convertToNativeInt(other)))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::operator%=(T other) noexcept -> SaturatingInteger & {
    _value = saturatingModulo(_value, convertToNativeInt(other));
    return *this;
}

}
