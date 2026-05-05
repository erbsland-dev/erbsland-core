// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::mem {

/// A small wrapper around a single byte.
/// Provides a convenient interface for working with individual bytes and bits.
class Byte final {
public:
    /// The raw byte value type.
    using Value = uint8_t;

public:
    /// Create a byte from a raw byte value.
    constexpr Byte(const Value byte) : _byte{byte} {} // NOLINT(*-explicit-constructor)

    // defaults
    Byte() = default;
    ~Byte() = default;
    Byte(const Byte &) = default;
    auto operator=(const Byte &) -> Byte & = default;
    Byte(Byte &&) = default;
    auto operator=(Byte &&) -> Byte & = default;

public:
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_byte, const Byte &other, other._byte);

public:
    /// Return the raw byte value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _byte; }
    /// Get the byte as an unsigned integer.
    [[nodiscard]] constexpr auto toUInt8() const noexcept -> uint8_t { return _byte; }
    /// Apply a bit mask to this byte.
    [[nodiscard]] constexpr auto masked(const uint8_t mask) const noexcept -> uint8_t { return _byte & mask; }
    /// Test if the masked bits equal the expected value.
    [[nodiscard]] constexpr auto matches(const uint8_t mask, const uint8_t expected) const noexcept -> bool {
        return masked(mask) == expected;
    }

private:
    Value _byte{0U}; /// The actual byte value.
};

}
