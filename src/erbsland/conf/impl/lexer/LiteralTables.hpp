// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Content.hpp"
#include "TokenType.hpp"

#include "../../../text/String.hpp"

#include <cstdint>
#include <unordered_map>
#include <variant>

/// Tables with literal constants of the language.
namespace erbsland::conf::impl::lexer::literal_tables {

/// Units accepted for time-delta literals.
enum class TimeDeltaUnit : std::uint8_t {
    Nanoseconds,
    Microseconds,
    Milliseconds,
    Seconds,
    Minutes,
    Hours,
    Days,
    Weeks,
    Months,
    Years,
};

/// Multiplication factor for a byte-count suffix.
/// @tested{LexerCoreIntegerTest}
struct ByteCountSuffix {
    std::int64_t factor; ///< Decimal or binary byte multiplier.
};

/// Unit selected by a time-delta suffix.
/// @tested{LexerAdvancedTimeDeltaTest}
struct TimeDeltaSuffix {
    TimeDeltaUnit unit; ///< Selected time unit.
};

/// Decoded byte-count or time-delta suffix.
using SuffixInfo = std::variant<ByteCountSuffix, TimeDeltaSuffix>;

/// Token data associated with a reserved identifier.
/// @tested{LexerCoreBooleanTest}
struct IdentifierInfo {
    TokenType type; ///< Literal token kind.
    Content value;  ///< Decoded literal value.
};

/// Reserved identifier lookup.
using IdentifierMap = std::unordered_map<text::String, IdentifierInfo>;
/// Integer suffix lookup.
using IntegerSuffixMap = std::unordered_map<text::String, SuffixInfo>;

/// Access the immutable literal lookup table, initialized on first use.
/// @return The shared lookup table.
/// @tested{LexerCoreBooleanTest LexerCoreIntegerTest LexerAdvancedTimeDeltaTest}
[[nodiscard]] auto identifierMap() -> const IdentifierMap &;

/// Access the immutable literal lookup table, initialized on first use.
/// @return The shared lookup table.
/// @tested{LexerCoreBooleanTest LexerCoreIntegerTest LexerAdvancedTimeDeltaTest}
[[nodiscard]] auto integerSuffixMap() -> const IntegerSuffixMap &;

}
