// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTypes.hpp"

#include <bit>
#include <concepts>
#include <cstddef>
#include <limits>
#include <span>

namespace erbsland::math {

/// An unsigned native integer supported by the compiler-safe bit operations.
template <typename T>
concept UnsignedNativeInteger = NativeInteger<T> && std::unsigned_integral<T>;

/// Rotate an unsigned integer to the left.
/// Rotation amounts are reduced modulo the bit width; negative amounts rotate to the right.
/// @tparam T An unsigned native integer type.
/// @param value The value to rotate.
/// @param amount The signed rotation amount.
/// @return The rotated value.
/// @tested{IntegerBitOperationsTest}
template <UnsignedNativeInteger T>
[[nodiscard]] constexpr auto rotateLeft(const T value, const int amount) noexcept -> T {
    return std::rotl(value, amount);
}

/// Rotate an unsigned integer to the right.
/// Rotation amounts are reduced modulo the bit width; negative amounts rotate to the left.
/// @tparam T An unsigned native integer type.
/// @param value The value to rotate.
/// @param amount The signed rotation amount.
/// @return The rotated value.
/// @tested{IntegerBitOperationsTest}
template <UnsignedNativeInteger T>
[[nodiscard]] constexpr auto rotateRight(const T value, const int amount) noexcept -> T {
    return std::rotr(value, amount);
}

/// Load an unsigned integer from bytes in big-endian order.
/// This function does not depend on alignment, aliasing, or native byte order.
/// @tparam T An unsigned native integer type.
/// @param bytes Exactly `sizeof(T)` bytes.
/// @return The decoded integer.
/// @tested{IntegerBitOperationsTest}
template <UnsignedNativeInteger T>
[[nodiscard]] constexpr auto loadBigEndian(const std::span<const std::byte, sizeof(T)> bytes) noexcept -> T {
    auto result = T{0};
    for (const auto byte : bytes) {
        result = static_cast<T>((result << 8U) | static_cast<T>(std::to_integer<unsigned int>(byte)));
    }
    return result;
}

/// Load an unsigned integer from bytes in little-endian order.
/// This function does not depend on alignment, aliasing, or native byte order.
/// @tparam T An unsigned native integer type.
/// @param bytes Exactly `sizeof(T)` bytes.
/// @return The decoded integer.
/// @tested{IntegerBitOperationsTest}
template <UnsignedNativeInteger T>
[[nodiscard]] constexpr auto loadLittleEndian(const std::span<const std::byte, sizeof(T)> bytes) noexcept -> T {
    auto result = T{0};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        result |= static_cast<T>(std::to_integer<unsigned int>(bytes[i])) << (i * 8U);
    }
    return result;
}

/// Store an unsigned integer as bytes in big-endian order.
/// This function does not depend on alignment, aliasing, or native byte order.
/// @tparam T An unsigned native integer type.
/// @param value The integer to encode.
/// @param bytes Exactly `sizeof(T)` writable bytes.
/// @tested{IntegerBitOperationsTest}
template <UnsignedNativeInteger T>
constexpr void storeBigEndian(const T value, const std::span<std::byte, sizeof(T)> bytes) noexcept {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto shift = (bytes.size() - i - 1U) * 8U;
        bytes[i] = std::byte{static_cast<unsigned char>(value >> shift)};
    }
}

/// Store an unsigned integer as bytes in little-endian order.
/// This function does not depend on alignment, aliasing, or native byte order.
/// @tparam T An unsigned native integer type.
/// @param value The integer to encode.
/// @param bytes Exactly `sizeof(T)` writable bytes.
/// @tested{IntegerBitOperationsTest}
template <UnsignedNativeInteger T>
constexpr void storeLittleEndian(const T value, const std::span<std::byte, sizeof(T)> bytes) noexcept {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = std::byte{static_cast<unsigned char>(value >> (i * 8U))};
    }
}

}
