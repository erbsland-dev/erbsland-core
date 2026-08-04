// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

#include <compare>
#include <cstdint>
#include <optional>

namespace erbsland::network {

/// A network port number.
/// Port zero selects an automatic local port and is invalid as a remote destination.
/// @tested{NetworkValueTest}
class Port final {
    constexpr static auto cMaximumPortNumber = 65535U;

public:
    /// Create the automatic port.
    constexpr Port() noexcept = default;
    /// Create a port from its numeric value.
    /// @param value The port number, where zero selects an automatic local port.
    explicit constexpr Port(const uint16_t value) noexcept : _value{value} {}

public: // operators
    /// Compare two port numbers.
    /// @param other The port to compare with this port.
    /// @return The strong ordering between the numeric values.
    [[nodiscard]] constexpr auto operator<=>(const Port &other) const noexcept -> std::strong_ordering = default;

public: // accessors
    /// Test if this port requests automatic local selection.
    /// @return `true` if the port number is zero.
    [[nodiscard]] constexpr auto isAutomatic() const noexcept -> bool { return _value == 0U; }
    /// Get the numeric port value.
    /// @return The port number.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> uint16_t { return _value; }

public: // conversion
    /// Format the decimal port number.
    /// @return The port text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse a decimal port number.
    /// @param text The complete decimal port text.
    /// @return The port, or `std::nullopt` if the text is outside `0` through `65535` or malformed.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<Port>;
    /// Parse a decimal port number.
    /// @param text The complete decimal port text.
    /// @return The parsed port.
    /// @throws err::ParseError If the text is not a valid port number.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> Port;

private:
    uint16_t _value{}; ///< The numeric port value.
};

}
