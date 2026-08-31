// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogLevelFormat.hpp"

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/EnumFlags.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The severity of a log entry.
/// @tested{LogCoreTest}
class LogLevel final {
public:
    /// The raw severity value and flag bit.
    enum class Value : uint8_t {
        Trace = 1U << 0U,                                        ///< Detailed diagnostics disabled by default.
        Information = 1U << 1U,                                  ///< Normal application progress and state.
        Warning = 1U << 2U,                                      ///< A recoverable problem or unexpected condition.
        Error = 1U << 3U,                                        ///< An operation failure requiring attention.
        All = (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U), ///< All available severity levels.
    };

    static constexpr auto Trace = Value::Trace;             ///< Detailed diagnostics disabled by default.
    static constexpr auto Information = Value::Information; ///< Normal application progress and state.
    static constexpr auto Warning = Value::Warning;         ///< A recoverable problem or unexpected condition.
    static constexpr auto Error = Value::Error;             ///< An operation failure requiring attention.
    static constexpr auto All = Value::All;                 ///< All available severity levels.

    /// Create the default information level.
    constexpr LogLevel() noexcept = default;
    /// Create a level from its raw value.
    /// @param value The raw severity value.
    constexpr LogLevel(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                               // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogLevel &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogLevel &other, value, other._value);

public: // accessors
    /// Get the raw severity value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this level to its canonical configuration identifier.
    /// @return The full lowercase severity identifier.
    [[nodiscard]] auto toString() const -> text::String;
    /// Render this level using a configured spelling and case.
    /// @param format The desired level representation.
    /// @return The rendered severity text, or `unknown` for an invalid value.
    [[nodiscard]] auto toString(LogLevelFormat format) const -> text::String;
    /// Get all supported configuration identifiers, including aliases.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a severity identifier case-insensitively using ASCII folding.
    /// The aliases `info` and `warn` are accepted in addition to canonical identifiers.
    /// @param text The identifier to parse.
    /// @return The parsed level, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogLevel>;
    /// Parse a severity identifier.
    /// @param text The identifier to parse.
    /// @return The parsed level.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogLevel;

private:
    Value _value{Information}; ///< Raw severity value.
};

/// A set of log severity levels.
using LogLevels = util::EnumFlags<LogLevel::Value>;

}
