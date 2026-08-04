// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <limits>
#include <type_traits>

namespace erbsland::math {

template <NativeInteger T>
BigUnsignedInteger::BigUnsignedInteger(const T value) {
    static_assert(sizeof(T) <= sizeof(std::uint64_t), "Native integers wider than 64 bits are not supported");
    if constexpr (std::signed_integral<T>) {
        if (value < 0) {
            throwNegativeValue();
        }
    }
    assign(static_cast<std::uint64_t>(value));
}

template <NativeInteger T>
auto BigUnsignedInteger::cast() const -> T {
    const auto maximum = BigUnsignedInteger{std::numeric_limits<T>::max()};
    if (*this > maximum) {
        return std::numeric_limits<T>::max();
    }
    return static_cast<T>(toUInt64Unchecked());
}

template <NativeInteger T>
auto BigUnsignedInteger::castOrThrow() const -> T {
    const auto maximum = BigUnsignedInteger{std::numeric_limits<T>::max()};
    if (*this > maximum) {
        throwCastOverflow();
    }
    return static_cast<T>(toUInt64Unchecked());
}

}
