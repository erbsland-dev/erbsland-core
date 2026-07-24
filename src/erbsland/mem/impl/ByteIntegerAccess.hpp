// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Byte.hpp"
#include "../ByteSpan.hpp"
#include "../Endianness.hpp"

#include <bit>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace erbsland::mem::impl {

/// Test whether an integer fits into a byte sequence at an offset.
/// @tparam T A non-boolean native integer type.
/// @param offset The first byte index.
/// @param length The available byte length.
/// @return `true` if the complete integer fits.
/// @tested{ByteArrayTest ByteBlockTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr auto isIntegerByteRangeValid(const std::size_t offset, const std::size_t length) noexcept
    -> bool {
    return offset <= length && sizeof(T) <= length - offset;
}

/// Decode an integer from an already validated byte range.
/// @tparam T A non-boolean native integer type.
/// @param bytes The source bytes.
/// @param offset The first byte index.
/// @param endianness The byte order.
/// @return The decoded integer.
/// @tested{ByteArrayTest ByteBlockTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr auto getIntegerUnchecked(
    const ConstByteSpan bytes, const std::size_t offset, const Endianness endianness) noexcept -> T {
    using Unsigned = std::make_unsigned_t<T>;
    auto result = Unsigned{0};
    for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
        const auto shift = endianness == Endianness::Little ? i * 8U : (sizeof(T) - i - 1U) * 8U;
        result = static_cast<Unsigned>(result | (static_cast<Unsigned>(bytes[offset + i].toUInt8()) << shift));
    }
    if constexpr (std::signed_integral<T>) {
        return std::bit_cast<T>(result);
    } else {
        return result;
    }
}

/// Encode an integer into an already validated byte range.
/// @tparam T A non-boolean native integer type.
/// @param bytes The destination bytes.
/// @param offset The first byte index.
/// @param value The integer value.
/// @param endianness The byte order.
/// @tested{ByteArrayTest ByteBlockTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
constexpr void setIntegerUnchecked(
    const ByteSpan bytes, const std::size_t offset, const T value, const Endianness endianness) noexcept {
    using Unsigned = std::make_unsigned_t<T>;
    const auto unsignedValue = [&]() constexpr -> Unsigned {
        if constexpr (std::signed_integral<T>) {
            return std::bit_cast<Unsigned>(value);
        } else {
            return value;
        }
    }();
    for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
        const auto shift = endianness == Endianness::Little ? i * 8U : (sizeof(T) - i - 1U) * 8U;
        bytes[offset + i] = Byte{static_cast<uint8_t>(unsignedValue >> shift)};
    }
}

}
