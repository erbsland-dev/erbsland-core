// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LexerToken.hpp"
#include "TokenType.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

/// Tables with literal constants of the language.
struct LiteralTables {
    /// Units accepted for time-delta literals.
    enum class TimeDeltaUnit : uint8_t {
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
    struct ByteCountSuffix {
        int64_t factor;
    };

    /// Unit selected by a time-delta suffix.
    struct TimeDeltaSuffix {
        TimeDeltaUnit unit;
    };

    using SuffixInfo = std::variant<ByteCountSuffix, TimeDeltaSuffix>;

    /// Token data associated with a reserved identifier.
    struct IdentifierInfo {
        TokenType type;
        Content value;
    };

    using IdentifierMap = std::unordered_map<text::String, IdentifierInfo>;
    using IntegerSuffixMap = std::unordered_map<text::String, SuffixInfo>;

    inline static const IdentifierMap identifierMap = {
        {"true"_el, IdentifierInfo{TokenType::Boolean, Content{true}}},
        {"yes"_el, IdentifierInfo{TokenType::Boolean, Content{true}}},
        {"enabled"_el, IdentifierInfo{TokenType::Boolean, Content{true}}},
        {"on"_el, IdentifierInfo{TokenType::Boolean, Content{true}}},
        {"false"_el, IdentifierInfo{TokenType::Boolean, Content{false}}},
        {"no"_el, IdentifierInfo{TokenType::Boolean, Content{false}}},
        {"disabled"_el, IdentifierInfo{TokenType::Boolean, Content{false}}},
        {"off"_el, IdentifierInfo{TokenType::Boolean, Content{false}}},
    };

    inline static const IntegerSuffixMap integerSuffixMap = {
        {"kb"_el, ByteCountSuffix{1000}},
        {"mb"_el, ByteCountSuffix{1000000}},
        {"gb"_el, ByteCountSuffix{1000000000}},
        {"tb"_el, ByteCountSuffix{1000000000000}},
        {"pb"_el, ByteCountSuffix{1000000000000000}},
        {"eb"_el, ByteCountSuffix{1000000000000000000}},
        {"zb"_el, ByteCountSuffix{-1}},
        {"yb"_el, ByteCountSuffix{-1}},
        {"kib"_el, ByteCountSuffix{1024}},
        {"mib"_el, ByteCountSuffix{1048576}},
        {"gib"_el, ByteCountSuffix{1073741824}},
        {"tib"_el, ByteCountSuffix{1099511627776}},
        {"pib"_el, ByteCountSuffix{1125899906842624}},
        {"eib"_el, ByteCountSuffix{1152921504606846976}},
        {"zib"_el, ByteCountSuffix{-1}},
        {"yib"_el, ByteCountSuffix{-1}},
        {"ns"_el, TimeDeltaSuffix{TimeDeltaUnit::Nanoseconds}},
        {"nanosecond"_el, TimeDeltaSuffix{TimeDeltaUnit::Nanoseconds}},
        {"nanoseconds"_el, TimeDeltaSuffix{TimeDeltaUnit::Nanoseconds}},
        {"us"_el, TimeDeltaSuffix{TimeDeltaUnit::Microseconds}},
        {"µs"_el, TimeDeltaSuffix{TimeDeltaUnit::Microseconds}},
        {"microsecond"_el, TimeDeltaSuffix{TimeDeltaUnit::Microseconds}},
        {"microseconds"_el, TimeDeltaSuffix{TimeDeltaUnit::Microseconds}},
        {"ms"_el, TimeDeltaSuffix{TimeDeltaUnit::Milliseconds}},
        {"millisecond"_el, TimeDeltaSuffix{TimeDeltaUnit::Milliseconds}},
        {"milliseconds"_el, TimeDeltaSuffix{TimeDeltaUnit::Milliseconds}},
        {"s"_el, TimeDeltaSuffix{TimeDeltaUnit::Seconds}},
        {"second"_el, TimeDeltaSuffix{TimeDeltaUnit::Seconds}},
        {"seconds"_el, TimeDeltaSuffix{TimeDeltaUnit::Seconds}},
        {"m"_el, TimeDeltaSuffix{TimeDeltaUnit::Minutes}},
        {"minute"_el, TimeDeltaSuffix{TimeDeltaUnit::Minutes}},
        {"minutes"_el, TimeDeltaSuffix{TimeDeltaUnit::Minutes}},
        {"h"_el, TimeDeltaSuffix{TimeDeltaUnit::Hours}},
        {"hour"_el, TimeDeltaSuffix{TimeDeltaUnit::Hours}},
        {"hours"_el, TimeDeltaSuffix{TimeDeltaUnit::Hours}},
        {"d"_el, TimeDeltaSuffix{TimeDeltaUnit::Days}},
        {"day"_el, TimeDeltaSuffix{TimeDeltaUnit::Days}},
        {"days"_el, TimeDeltaSuffix{TimeDeltaUnit::Days}},
        {"w"_el, TimeDeltaSuffix{TimeDeltaUnit::Weeks}},
        {"week"_el, TimeDeltaSuffix{TimeDeltaUnit::Weeks}},
        {"weeks"_el, TimeDeltaSuffix{TimeDeltaUnit::Weeks}},
        {"month"_el, TimeDeltaSuffix{TimeDeltaUnit::Months}},
        {"months"_el, TimeDeltaSuffix{TimeDeltaUnit::Months}},
        {"year"_el, TimeDeltaSuffix{TimeDeltaUnit::Years}},
        {"years"_el, TimeDeltaSuffix{TimeDeltaUnit::Years}},
    };
};

}
