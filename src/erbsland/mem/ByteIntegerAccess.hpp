// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteSpan.hpp"
#include "Endianness.hpp"

#include "impl/ByteIntegerAccess.hpp"
#include "impl/Throw.hpp"

#include "../unit/ByteIndex.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::mem {

/// Get an integer from a byte span or return a default value for an invalid range.
/// @tparam T A non-boolean native integer type.
/// @param bytes The source bytes.
/// @param offset The first byte index.
/// @param endianness The byte order.
/// @param defaultOnError The value returned for an invalid range.
/// @return The decoded value, or `defaultOnError`.
/// @tested{ByteArrayTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr auto getInteger(
    const ConstByteSpan bytes,
    const unit::ByteIndex offset,
    const Endianness endianness = Endianness::Little,
    const T defaultOnError = T()) noexcept -> T {
    if (!offset.isValid() || !impl::isIntegerByteRangeValid<T>(offset.toSizeT(), bytes.size())) {
        return defaultOnError;
    }
    return impl::getIntegerUnchecked<T>(bytes, offset.toSizeT(), endianness);
}

/// Get an integer from a byte span or throw for an invalid range.
/// @tparam T A non-boolean native integer type.
/// @param bytes The source bytes.
/// @param offset The first byte index.
/// @param endianness The byte order.
/// @return The decoded value.
/// @throws err::OutOfRangeError If the integer does not fit at `offset`.
/// @tested{ByteArrayTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr auto getIntegerOrThrow(
    const ConstByteSpan bytes, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) -> T {
    if (!offset.isValid() || !impl::isIntegerByteRangeValid<T>(offset.toSizeT(), bytes.size())) {
        impl::throwOutOfRange("Integer byte range out of range");
    }
    return impl::getIntegerUnchecked<T>(bytes, offset.toSizeT(), endianness);
}

/// Decode an integer from a byte span into an existing value.
/// The output remains unchanged if the byte range is invalid.
/// @tparam T A non-boolean native integer type.
/// @param bytes The source bytes.
/// @param value The destination value.
/// @param offset The first byte index.
/// @param endianness The byte order.
/// @return `true` on success.
/// @tested{ByteArrayTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr auto getIntegerInto(
    const ConstByteSpan bytes,
    T &value,
    const unit::ByteIndex offset,
    const Endianness endianness = Endianness::Little) noexcept -> bool {
    if (!offset.isValid() || !impl::isIntegerByteRangeValid<T>(offset.toSizeT(), bytes.size())) {
        return false;
    }
    value = impl::getIntegerUnchecked<T>(bytes, offset.toSizeT(), endianness);
    return true;
}

/// Store an integer in a byte span.
/// The bytes remain unchanged if the byte range is invalid.
/// @tparam T A non-boolean native integer type.
/// @param bytes The destination bytes.
/// @param offset The first byte index.
/// @param value The integer value.
/// @param endianness The byte order.
/// @return `true` on success.
/// @tested{ByteArrayTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr auto setInteger(
    const ByteSpan bytes,
    const unit::ByteIndex offset,
    const T value,
    const Endianness endianness = Endianness::Little) noexcept -> bool {
    if (!offset.isValid() || !impl::isIntegerByteRangeValid<T>(offset.toSizeT(), bytes.size())) {
        return false;
    }
    impl::setIntegerUnchecked(bytes, offset.toSizeT(), value, endianness);
    return true;
}

/// Store an integer in a byte span or throw for an invalid range.
/// @tparam T A non-boolean native integer type.
/// @param bytes The destination bytes.
/// @param offset The first byte index.
/// @param value The integer value.
/// @param endianness The byte order.
/// @throws err::OutOfRangeError If the integer does not fit at `offset`.
/// @tested{ByteArrayTest}
template <typename T>
    requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
constexpr void setIntegerOrThrow(
    const ByteSpan bytes,
    const unit::ByteIndex offset,
    const T value,
    const Endianness endianness = Endianness::Little) {
    if (!offset.isValid() || !impl::isIntegerByteRangeValid<T>(offset.toSizeT(), bytes.size())) {
        impl::throwOutOfRange("Integer byte range out of range");
    }
    impl::setIntegerUnchecked(bytes, offset.toSizeT(), value, endianness);
}

}
