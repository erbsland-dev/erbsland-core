// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerConversion.hpp"
#include "IntegerTraits.hpp"

namespace erbsland::math {

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::cast() const noexcept -> SaturatingInteger<NativeIntegerOfT<T>> {
    using Target = NativeIntegerOfT<T>;
    return SaturatingInteger<Target>{_value};
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::castOrThrow() const -> SaturatingInteger<NativeIntegerOfT<T>> {
    using Target = NativeIntegerOfT<T>;
    if (willCastOverflow<Target>(_value)) {
        impl::throwOverflow("Saturating integer value cannot be represented by the target type");
    }
    return SaturatingInteger<Target>{_value};
}

template <NativeInteger tValue>
constexpr auto SaturatingInteger<tValue>::toSizeT() const noexcept -> std::size_t {
    return cast<std::size_t>().toRawValue();
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr void SaturatingInteger<tValue>::clamp(T minimum, T maximum) noexcept {
    const auto cMin = saturatingCast<NativeValue>(convertToNativeInt(minimum));
    if (_value < cMin) {
        _value = cMin;
    }
    const auto cMax = saturatingCast<NativeValue>(convertToNativeInt(maximum));
    if (_value > cMax) {
        _value = cMax;
    }
}

template <NativeInteger tValue>
template <NativeInteger T>
constexpr void SaturatingInteger<tValue>::clamp(IntegerRange<T> range) noexcept {
    clamp(range.minimum(), range.maximum());
}

template <NativeInteger tValue>
template <AnyIntegerType T>
constexpr auto SaturatingInteger<tValue>::clamped(T minimum, T maximum) const noexcept -> SaturatingInteger {
    const auto cMin = toSaturatingInteger(minimum).template cast<NativeValue>();
    if (*this < cMin) {
        return cMin;
    }
    const auto cMax = toSaturatingInteger(maximum).template cast<NativeValue>();
    if (*this > cMax) {
        return cMax;
    }
    return *this;
}

template <NativeInteger tValue>
template <NativeInteger T>
constexpr auto SaturatingInteger<tValue>::clamped(IntegerRange<T> range) const noexcept -> SaturatingInteger {
    return clamped(range.minimum(), range.maximum());
}

template <NativeInteger tValue>
template <AnyIntegerType T>
void SaturatingInteger<tValue>::wrap(T minimum, T maximum) noexcept {
    *this = wrapped(minimum, maximum);
}

template <NativeInteger tValue>
template <NativeInteger T>
void SaturatingInteger<tValue>::wrap(IntegerRange<T> range) noexcept {
    *this = wrapped(range);
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wrapped(T minimum, T maximum) const noexcept -> SaturatingInteger {
    const auto nativeMinimum = convertToNativeInt(minimum);
    const auto nativeMaximum = convertToNativeInt(maximum);
    if (mixedIntegerCompare(nativeMinimum, nativeMaximum) == std::strong_ordering::greater) {
        return {};
    }
    return wrapped(IntegerRange<NativeIntegerOfT<T>>{nativeMinimum, nativeMaximum});
}

template <NativeInteger tValue>
template <NativeInteger T>
auto SaturatingInteger<tValue>::wrapped(IntegerRange<T> range) const noexcept -> SaturatingInteger {
    return std::get<0>(wrappedAndCount(range));
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wrapAndCount(T minimum, T maximum) noexcept -> WrapCount {
    const auto [wrappedValue, wrapCount] = wrappedAndCount(minimum, maximum);
    _value = wrappedValue._value;
    return wrapCount;
}

template <NativeInteger tValue>
template <NativeInteger T>
auto SaturatingInteger<tValue>::wrapAndCount(IntegerRange<T> range) noexcept -> WrapCount {
    const auto [wrappedValue, wrapCount] = wrappedAndCount(range);
    _value = wrappedValue._value;
    return wrapCount;
}

template <NativeInteger tValue>
template <AnyIntegerType T>
auto SaturatingInteger<tValue>::wrappedAndCount(T minimum, T maximum) const noexcept
    -> std::tuple<SaturatingInteger, WrapCount> {
    const auto nativeMinimum = convertToNativeInt(minimum);
    const auto nativeMaximum = convertToNativeInt(maximum);
    if (mixedIntegerCompare(nativeMinimum, nativeMaximum) == std::strong_ordering::greater) {
        return {SaturatingInteger{}, WrapCount{}};
    }
    return wrappedAndCount(IntegerRange<NativeIntegerOfT<T>>{nativeMinimum, nativeMaximum});
}

template <NativeInteger tValue>
template <NativeInteger T>
auto SaturatingInteger<tValue>::wrappedAndCount(IntegerRange<T> range) const noexcept
    -> std::tuple<SaturatingInteger, WrapCount> {
    if (!SaturatingInteger::range().contains(range)) {
        return {SaturatingInteger{}, WrapCount{}};
    }

    const auto nativeRange = range.template cast<NativeValue>();
    if (SaturatingInteger::range() == nativeRange) {
        return {*this, WrapCount{}};
    }

    using UnsignedValue = std::make_unsigned_t<NativeValue>;
    const auto minimum = nativeRange.minimum();
    const auto maximum = nativeRange.maximum();
    const auto distance = integerAbsoluteDifference(_value, minimum);
    const auto valueCount = static_cast<UnsignedValue>(integerAbsoluteDifference(maximum, minimum) + UnsignedValue{1});
    const auto plainRemainder = static_cast<UnsignedValue>(distance % valueCount);

    auto wrapCount = WrapCount{};
    auto remainder = UnsignedValue{};
    if (mixedIntegerCompare(_value, minimum) == std::strong_ordering::less) {
        remainder = (plainRemainder == UnsignedValue{0}) ? UnsignedValue{0}
                                                         : static_cast<UnsignedValue>(valueCount - plainRemainder);
        auto quotientMagnitude = static_cast<UnsignedValue>(distance / valueCount);
        if (plainRemainder != UnsignedValue{0}) {
            ++quotientMagnitude;
        }
        wrapCount.subtract(quotientMagnitude);
    } else {
        remainder = plainRemainder;
        wrapCount = WrapCount{static_cast<UnsignedValue>(distance / valueCount)};
    }

    auto wrappedValue = SaturatingInteger{minimum};
    wrappedValue.add(remainder);
    return {wrappedValue, wrapCount};
}

template <NativeInteger tValue>
constexpr auto SaturatingInteger<tValue>::isNegative() const noexcept -> bool {
    if constexpr (std::signed_integral<NativeValue>) {
        return _value < 0;
    } else {
        return false;
    }
}

template <NativeInteger tValue>
constexpr auto SaturatingInteger<tValue>::toAbsolute() const noexcept -> SaturatingInteger {
    if constexpr (std::unsigned_integral<NativeValue>) {
        return *this;
    } else if (isMinimum()) {
        return maximum();
    } else if (_value < 0) {
        return SaturatingInteger{static_cast<NativeValue>(-_value)};
    } else {
        return *this;
    }
}

template <NativeInteger tValue>
constexpr auto SaturatingInteger<tValue>::toUnsignedAbsolute() const noexcept
    -> SaturatingInteger<std::make_unsigned_t<NativeValue>> {
    if constexpr (std::unsigned_integral<NativeValue>) {
        return *this;
    } else {
        return SaturatingInteger<decltype(math::toUnsignedAbsolute(_value))>{math::toUnsignedAbsolute(_value)};
    }
}

template <NativeInteger tValue>
constexpr auto SaturatingInteger<tValue>::negated() const noexcept -> SaturatingInteger {
    if constexpr (std::unsigned_integral<NativeValue>) {
        return {};
    } else if (isMinimum()) {
        return maximum();
    } else {
        return SaturatingInteger{static_cast<NativeValue>(-_value)};
    }
}

template <NativeInteger tValue>
void SaturatingInteger<tValue>::negate() noexcept {
    if constexpr (std::unsigned_integral<NativeValue>) {
        _value = 0;
    } else {
        _value = (isMinimum() ? std::numeric_limits<NativeValue>::max() : static_cast<NativeValue>(-_value));
    }
}

}
