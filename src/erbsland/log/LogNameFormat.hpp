// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The representation used for a rendered stream name.
/// @tested{LogCoreTest}
class LogNameFormat final {
public:
    /// The raw name-format value.
    enum Value : uint8_t {
        Full,          ///< The complete hierarchical path.
        Leaf,          ///< Only the final path segment.
        HeadAndLeaf,   ///< The first and final path segments.
        LeftTruncated, ///< The full path truncated on its left side.
    };

    /// Create the default full-name format.
    constexpr LogNameFormat() noexcept = default;
    /// Create a name format from its raw value.
    /// @param value The raw name-format value.
    constexpr LogNameFormat(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                    // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogNameFormat &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogNameFormat &other, value, other._value);

public: // accessors
    /// Get the raw name-format value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this format to its canonical configuration identifier.
    /// @return The lowercase format identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical name-format identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed format, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogNameFormat>;
    /// Parse a canonical name-format identifier.
    /// @param text The identifier to parse.
    /// @return The parsed format.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogNameFormat;

private:
    Value _value{Full}; ///< Raw name-format value.
};

}
