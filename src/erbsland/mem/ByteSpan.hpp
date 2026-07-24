// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::mem {

/// A writable dynamic-extent span of bytes.
/// @tested{ByteArrayTest}
using ByteSpan = std::span<Byte>;

/// A read-only dynamic-extent span of bytes.
/// @tested{ByteArrayTest}
using ConstByteSpan = std::span<const Byte>;

/// A writable fixed-extent span of bytes.
/// @tparam Extent The number of bytes in the span.
/// @tested{ByteArrayTest}
template <std::size_t Extent>
using FixedByteSpan = std::span<Byte, Extent>;

/// A read-only fixed-extent span of bytes.
/// @tparam Extent The number of bytes in the span.
/// @tested{ByteArrayTest}
template <std::size_t Extent>
using FixedConstByteSpan = std::span<const Byte, Extent>;

/// Create a zero-copy byte view of mutable standard-byte storage.
/// @tested{ByteArrayTest}
[[nodiscard]] auto toByteSpan(std::span<std::byte> span) noexcept -> ByteSpan;
/// Create a zero-copy byte view of mutable unsigned-byte storage.
/// @tested{ByteArrayTest}
[[nodiscard]] auto toByteSpan(std::span<uint8_t> span) noexcept -> ByteSpan;
/// Create a zero-copy byte view of mutable character storage.
/// @tested{ByteArrayTest}
[[nodiscard]] auto toByteSpan(std::span<char> span) noexcept -> ByteSpan;
/// Create a zero-copy read-only byte view of standard-byte storage.
/// @tested{ByteArrayTest}
[[nodiscard]] auto toConstByteSpan(std::span<const std::byte> span) noexcept -> ConstByteSpan;
/// Create a zero-copy read-only byte view of unsigned-byte storage.
/// @tested{ByteArrayTest}
[[nodiscard]] auto toConstByteSpan(std::span<const uint8_t> span) noexcept -> ConstByteSpan;
/// Create a zero-copy read-only byte view of character storage.
/// @tested{ByteArrayTest}
[[nodiscard]] auto toConstByteSpan(std::span<const char> span) noexcept -> ConstByteSpan;

}
