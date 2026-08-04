// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <limits>
#include <type_traits>

namespace erbsland::math {

template <NativeInteger T>
BigInteger::BigInteger(const T value) {
    static_assert(sizeof(T) <= sizeof(std::uint64_t), "Native integers wider than 64 bits are not supported");
    if constexpr (std::signed_integral<T>) {
        _negative = value < 0;
        _magnitude = BigUnsignedInteger{toUnsignedAbsolute(value)};
    } else {
        _magnitude = BigUnsignedInteger{value};
    }
}

template <NativeInteger T>
auto BigInteger::cast() const -> T {
    if (!_negative) {
        return _magnitude.template cast<T>();
    }
    if constexpr (std::unsigned_integral<T>) {
        return T{0};
    } else {
        const auto minimumMagnitude = BigUnsignedInteger{toUnsignedAbsolute(std::numeric_limits<T>::min())};
        if (_magnitude >= minimumMagnitude) {
            return std::numeric_limits<T>::min();
        }
        const auto magnitudeValue = _magnitude.template cast<std::make_unsigned_t<T>>();
        return static_cast<T>(-static_cast<T>(magnitudeValue));
    }
}

template <NativeInteger T>
auto BigInteger::castOrThrow() const -> T {
    if (!_negative) {
        return _magnitude.template castOrThrow<T>();
    }
    if constexpr (std::unsigned_integral<T>) {
        throwCastOverflow();
    } else {
        const auto minimumMagnitude = BigUnsignedInteger{toUnsignedAbsolute(std::numeric_limits<T>::min())};
        if (_magnitude > minimumMagnitude) {
            throwCastOverflow();
        }
        if (_magnitude == minimumMagnitude) {
            return std::numeric_limits<T>::min();
        }
        const auto magnitudeValue = _magnitude.template castOrThrow<std::make_unsigned_t<T>>();
        return static_cast<T>(-static_cast<T>(magnitudeValue));
    }
}

}
