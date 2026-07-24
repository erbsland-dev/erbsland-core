// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <compare>
#include <cstdint>

namespace erbsland::network {

/// A numeric IPv6 scope or network-interface identifier.
/// @tested{NetworkValueTest}
class ScopeId final {
public:
    /// Create an unspecified scope identifier.
    constexpr ScopeId() noexcept = default;
    /// Create a scope identifier from a numeric interface identifier.
    /// @param value The scope value, where zero means unspecified.
    explicit constexpr ScopeId(const uint32_t value) noexcept : _value{value} {}

public: // operators
    /// Compare two scope identifiers.
    /// @param other The scope identifier to compare with this value.
    /// @return The strong ordering between the numeric values.
    [[nodiscard]] constexpr auto operator<=>(const ScopeId &other) const noexcept -> std::strong_ordering = default;

public: // accessors
    /// Test if a scope identifier is specified.
    /// @return `true` if the numeric value is nonzero.
    [[nodiscard]] constexpr auto isSpecified() const noexcept -> bool { return _value != 0U; }
    /// Get the numeric scope identifier.
    /// @return The scope value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> uint32_t { return _value; }

private:
    uint32_t _value{}; ///< The numeric scope value.
};

}
