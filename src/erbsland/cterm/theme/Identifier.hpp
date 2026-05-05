// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>

namespace erbsland::cterm::theme {

/// The logical kind of a theme identifier.
enum class IdentifierType : uint8_t {
    Element, ///< Theme element identifiers.
    Part,    ///< Theme part identifiers.
};

/// A strongly typed numeric theme identifier.
/// @tparam tIdentifierType The logical identifier kind.
template <IdentifierType tIdentifierType>
class Identifier final {
public:
    /// Create an invalid identifier.
    constexpr Identifier() noexcept = default;
    /// Create an identifier from a raw value.
    /// @param value The raw identifier value.
    explicit constexpr Identifier(const uint16_t value) noexcept : _value{value} {}

public: // operators
    /// Compare two identifiers.
    auto operator==(const Identifier &other) const noexcept -> bool = default;
    /// Compare two identifiers.
    auto operator!=(const Identifier &other) const noexcept -> bool = default;

public:
    /// Get the raw identifier value.
    /// @return The raw identifier value.
    [[nodiscard]] constexpr auto value() const noexcept -> uint16_t { return _value; }
    /// Test if this identifier is non-empty.
    /// @return `true` if this identifier has a positive value.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _value != 0; }
    /// Get a stable hash for this identifier.
    /// @return The hash value.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t { return static_cast<std::size_t>(_value); }

private:
    uint16_t _value{0}; ///< The raw identifier value.
};

}
