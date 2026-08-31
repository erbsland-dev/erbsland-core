// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The spelling and case used for a rendered level.
/// @tested{LogCoreTest}
class LogLevelFormat final {
public:
    /// The raw level-format value.
    enum Value : uint8_t {
        ThreeLetterUpper, ///< Three uppercase letters.
        ShortLower,       ///< Three lowercase letters.
        FullLower,        ///< Full lowercase name.
        FullUpper,        ///< Full uppercase name.
    };

    /// Create the default three-letter uppercase format.
    constexpr LogLevelFormat() noexcept = default;
    /// Create a level format from its raw value.
    /// @param value The raw level-format value.
    constexpr LogLevelFormat(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                     // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogLevelFormat &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogLevelFormat &other, value, other._value);

public: // accessors
    /// Get the raw level-format value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this format to its canonical configuration identifier.
    /// @return The lowercase format identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical level-format identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed format, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogLevelFormat>;
    /// Parse a canonical level-format identifier.
    /// @param text The identifier to parse.
    /// @return The parsed format.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogLevelFormat;

private:
    Value _value{ThreeLetterUpper}; ///< Raw level-format value.
};

}
