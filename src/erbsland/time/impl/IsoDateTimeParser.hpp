// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParsedIsoDateTime.hpp"

#include "../DateTimePrecision.hpp"
#include "../TimeAmounts.hpp"

#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"

#include <optional>

namespace erbsland::time::impl {

/// Strict parser for the supported ISO date/time subset.
///
/// Parses ISO 8601 date/time strings with full validation and precision detection.
/// @tested{DateTimeTest}
class IsoDateTimeParser final {
public:
    /// Parse an ISO date/time string.
    /// @param text The text to parse.
    /// @param allowOffset Whether to allow an explicit UTC offset in the text.
    /// @return The parsed fields, or `std::nullopt` on failure.
    [[nodiscard]] static auto parse(const text::String &text, bool allowOffset) noexcept
        -> std::optional<ParsedIsoDateTime>;

private:
    explicit IsoDateTimeParser(const text::String &text) noexcept;

    /// Parse the date/time string.
    /// @param allowOffset Whether to allow an explicit UTC offset.
    /// @return The parsed fields, or `std::nullopt` on failure.
    [[nodiscard]] auto parse(bool allowOffset) noexcept -> std::optional<ParsedIsoDateTime>;
    /// Parse the date portion.
    /// @return `true` if parsing succeeded.
    [[nodiscard]] auto parseDate() noexcept -> bool;
    /// Parse the time portion.
    /// @return `true` if parsing succeeded.
    [[nodiscard]] auto parseTime() noexcept -> bool;
    /// Parse the fractional seconds.
    /// @return `true` if a fraction was found and parsed.
    [[nodiscard]] auto parseFraction() noexcept -> bool;
    /// Parse the UTC offset.
    /// @return `true` if an offset was found and parsed.
    [[nodiscard]] auto parseOffset() noexcept -> bool;
    /// Read a fixed number of digits.
    /// @param count The number of digits to read.
    /// @param value The parsed value is written here.
    /// @return `true` if all digits were read.
    [[nodiscard]] auto readNDigits(unit::CpLength count, int &value) noexcept -> bool;

private:
    text::StringCharReader _reader;
    int _year{0};
    int _month{1};
    int _day{1};
    int _hour{0};
    int _minute{0};
    int _second{0};
    int _nanosecond{0};
    DateTimePrecision _precision{DateTimePrecision::Year};
    bool _hasOffset{false};
    Seconds _offset;
};

}
