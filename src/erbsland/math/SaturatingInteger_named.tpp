// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerConversion.hpp"
#include "IntegerTraits.hpp"

namespace erbsland::math {

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::added(T other) const noexcept -> SaturatingInteger {
    return SaturatingInteger{saturatingAdd(_value, convertToNativeInt(other))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::subtracted(T other) const noexcept -> SaturatingInteger {
    return SaturatingInteger{saturatingSubtract(_value, convertToNativeInt(other))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::absoluteDifference(T other) const noexcept -> SaturatingInteger {
    const auto nativeOther = convertToNativeInt(other);
    if (_value >= nativeOther) {
        return SaturatingInteger{saturatingSubtract(_value, nativeOther)};
    }
    return SaturatingInteger{saturatingSubtract(nativeOther, _value)};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::multiplied(T other) const noexcept -> SaturatingInteger {
    return SaturatingInteger{saturatingMultiply(_value, convertToNativeInt(other))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::divided(T other) const noexcept -> SaturatingInteger {
    return SaturatingInteger{saturatingDivide(_value, convertToNativeInt(other))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::modulo(T other) const noexcept -> SaturatingInteger {
    return SaturatingInteger{saturatingModulo(_value, convertToNativeInt(other))};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
void SaturatingInteger<tValue>::add(T other) noexcept {
    _value = saturatingAdd(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
void SaturatingInteger<tValue>::subtract(T other) noexcept {
    _value = saturatingSubtract(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
void SaturatingInteger<tValue>::multiply(T other) noexcept {
    _value = saturatingMultiply(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
void SaturatingInteger<tValue>::divide(T other) noexcept {
    _value = saturatingDivide(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
void SaturatingInteger<tValue>::applyModulo(T other) noexcept {
    _value = saturatingModulo(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::divideGetRemainder(T other) noexcept -> SaturatingInteger {
    const auto nativeOther = convertToNativeInt(other);
    const auto quotient = saturatingDivide(_value, nativeOther);
    const auto remainder = saturatingModulo(_value, nativeOther);
    _value = quotient;
    return SaturatingInteger{remainder};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::divideKeepRemainder(T other) noexcept -> SaturatingInteger {
    const auto nativeOther = convertToNativeInt(other);
    const auto quotient = saturatingDivide(_value, nativeOther);
    const auto remainder = saturatingModulo(_value, nativeOther);
    _value = remainder;
    return SaturatingInteger{quotient};
}

template <NativeInteger tValue>
template <AnyIntegerType tFirst, AnyIntegerType tSecond>
    requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
auto SaturatingInteger<tValue>::fromAddition(tFirst first, tSecond second) noexcept -> SaturatingInteger {
    using C = CompatibleNativeIntegerT<NativeIntegerOfT<tFirst>, NativeIntegerOfT<tSecond>>;
    return SaturatingInteger<C>{convertToNativeInt(first)}
        .added(convertToNativeInt(second))
        .template cast<NativeValue>();
}

template <NativeInteger tValue>
template <AnyIntegerType tFirst, AnyIntegerType tSecond>
    requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
auto SaturatingInteger<tValue>::fromSubtraction(tFirst first, tSecond second) noexcept -> SaturatingInteger {
    using C = CompatibleNativeIntegerT<NativeIntegerOfT<tFirst>, NativeIntegerOfT<tSecond>>;
    return SaturatingInteger<C>{convertToNativeInt(first)}
        .subtracted(convertToNativeInt(second))
        .template cast<NativeValue>();
}

template <NativeInteger tValue>
template <AnyIntegerType tFirst, AnyIntegerType tSecond>
    requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
auto SaturatingInteger<tValue>::fromMultiplication(tFirst first, tSecond second) noexcept -> SaturatingInteger {
    using C = CompatibleNativeIntegerT<NativeIntegerOfT<tFirst>, NativeIntegerOfT<tSecond>>;
    return SaturatingInteger<C>{convertToNativeInt(first)}
        .multiplied(convertToNativeInt(second))
        .template cast<NativeValue>();
}

template <NativeInteger tValue>
template <AnyIntegerType tFirst, AnyIntegerType tSecond>
    requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
auto SaturatingInteger<tValue>::fromDivision(tFirst first, tSecond second) noexcept -> SaturatingInteger {
    using C = CompatibleNativeIntegerT<NativeIntegerOfT<tFirst>, NativeIntegerOfT<tSecond>>;
    return SaturatingInteger<C>{convertToNativeInt(first)}
        .divided(convertToNativeInt(second))
        .template cast<NativeValue>();
}

template <NativeInteger tValue>
template <AnyIntegerType tFirst, AnyIntegerType tSecond>
    requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
auto SaturatingInteger<tValue>::fromModulo(tFirst first, tSecond second) noexcept -> SaturatingInteger {
    using C = CompatibleNativeIntegerT<NativeIntegerOfT<tFirst>, NativeIntegerOfT<tSecond>>;
    return SaturatingInteger<C>{convertToNativeInt(first)}
        .modulo(convertToNativeInt(second))
        .template cast<NativeValue>();
}

template <NativeInteger tValue>
template <AnyIntegerType tFirst, AnyIntegerType tSecond>
    requires SignCompatibleIntegerOperandPair<tValue, tFirst, tSecond>
auto SaturatingInteger<tValue>::fromDivisionWithRemainder(tFirst first, tSecond second) noexcept
    -> std::tuple<SaturatingInteger, SaturatingInteger> {
    return std::make_tuple(fromDivision(first, second), fromModulo(first, second));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::raisedTo(T power) const noexcept -> SaturatingInteger {
    SaturatingInteger result{1};
    if (power == 0) {
        return result;
    }
    if constexpr (std::signed_integral<NativeIntegerOfT<T>>) {
        if (convertToNativeInt(power) < 0) {
            return SaturatingInteger{0};
        }
    }
    auto nativePower = math::toUnsignedAbsolute(toNativeInteger(power));
    auto base = *this;
    while (true) {
        if (nativePower & 1) {
            result *= base;
        }
        nativePower >>= 1;
        if (nativePower == 0) {
            break;
        }
        base *= base;
    }
    return result;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wouldAddSaturate(T other) const noexcept -> bool {
    return math::willAddOverflow(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wouldSubtractSaturate(T other) const noexcept -> bool {
    return math::willSubtractOverflow(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wouldMultiplySaturate(T other) const noexcept -> bool {
    return math::willMultiplyOverflow(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wouldDivideSaturate(T other) const noexcept -> bool {
    return math::willDivideOverflow(_value, convertToNativeInt(other));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wouldModuloSaturate(T other) const noexcept -> bool {
    return math::willModuloOverflow(_value, convertToNativeInt(other));
}

}
