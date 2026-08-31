// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The rotation schedule used for a log file.
/// @tested{LogCoreTest}
class LogFileRotation final {
public:
    /// The raw rotation value.
    enum Value : uint8_t {
        None,   ///< Never rotate the file.
        Hourly, ///< Rotate when the entry's UTC hour changes.
        Daily,  ///< Rotate when the entry's UTC calendar day changes.
        Weekly, ///< Rotate when the entry's UTC calendar week changes.
        Size,   ///< Rotate before the configured size would be exceeded.
    };

    /// Create the default no-rotation value.
    constexpr LogFileRotation() noexcept = default;
    /// Create a rotation mode from its raw value.
    /// @param value The raw rotation value.
    constexpr LogFileRotation(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                      // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogFileRotation &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogFileRotation &other, value, other._value);

public: // accessors
    /// Get the raw rotation value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this rotation mode to its canonical configuration identifier.
    /// @return The lowercase rotation identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical rotation identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed rotation, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogFileRotation>;
    /// Parse a canonical rotation identifier.
    /// @param text The identifier to parse.
    /// @return The parsed rotation.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogFileRotation;

private:
    Value _value{None}; ///< Raw rotation value.
};

}
