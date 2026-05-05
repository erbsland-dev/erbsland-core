// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IsoDateTimeParser.hpp"

#include <cstddef>

namespace erbsland::time::impl {

auto IsoDateTimeParser::parse(const text::StringView &text, const bool allowOffset) noexcept
    -> std::optional<ParsedIsoDateTime> {
    return IsoDateTimeParser{text}.parse(allowOffset);
}

IsoDateTimeParser::IsoDateTimeParser(const text::StringView &text) noexcept : _reader{text} {
}

auto IsoDateTimeParser::parse(const bool allowOffset) noexcept -> std::optional<ParsedIsoDateTime> {
    static const auto dateTimeSeparator = text::CharSet{U'T', U' '};
    if (!parseDate()) {
        return std::nullopt;
    }
    if (_reader.advanceIf(dateTimeSeparator)) {
        if (_precision < DateTimePrecision::Day) {
            return std::nullopt;
        }
        if (!parseTime()) {
            return std::nullopt;
        }
    }
    if (!_reader.isAtEnd()) {
        if (!allowOffset || _precision < DateTimePrecision::Hour || !parseOffset()) {
            return std::nullopt;
        }
    }
    if (!_reader.isAtEnd()) {
        return std::nullopt;
    }
    auto date = Date::fromYearMonthDay(_year, _month, _day);
    if (!date.isValid() || _hour > 23 || _minute > 59 || _second > 59) {
        return std::nullopt;
    }
    return ParsedIsoDateTime{
        .date = date,
        .time =
            Time{
                Hour{static_cast<int8_t>(_hour)},
                Minute{static_cast<int8_t>(_minute)},
                Second{static_cast<int8_t>(_second)},
                Nanoseconds{_nanosecond}},
        .precision = _precision,
        .hasOffset = _hasOffset,
        .offset = _offset};
}

auto IsoDateTimeParser::parseDate() noexcept -> bool {
    if (!readNDigits(unit::CpLength{4U}, _year)) {
        return false;
    }
    const auto extended = _reader.advanceIf(U'-');
    auto current = _reader.peek();
    if (extended && !current.isAsciiDigit()) {
        return false;
    }
    if (!current.isAsciiDigit()) {
        return true;
    }
    if (!readNDigits(unit::CpLength{2U}, _month)) {
        return false;
    }
    _precision = DateTimePrecision::Month;
    current = _reader.peek();
    if (extended) {
        if (!_reader.advanceIf(U'-')) {
            return true;
        }
        current = _reader.peek();
        if (!current.isAsciiDigit()) {
            return false;
        }
    } else if (!current.isAsciiDigit()) {
        return true;
    }
    if (!readNDigits(unit::CpLength{2U}, _day)) {
        return false;
    }
    _precision = DateTimePrecision::Day;
    return true;
}

auto IsoDateTimeParser::parseTime() noexcept -> bool {
    static const auto fractionSeparator = text::CharSet{U'.', U','};
    if (!readNDigits(unit::CpLength{2U}, _hour)) {
        return false;
    }
    _precision = DateTimePrecision::Hour;
    const auto extended = _reader.advanceIf(U':');
    auto current = _reader.peek();
    if (extended && !current.isAsciiDigit()) {
        return false;
    }
    if (!current.isAsciiDigit()) {
        return true;
    }
    if (!readNDigits(unit::CpLength{2U}, _minute)) {
        return false;
    }
    _precision = DateTimePrecision::Minute;
    current = _reader.peek();
    if (extended) {
        if (!_reader.advanceIf(U':')) {
            return true;
        }
        current = _reader.peek();
        if (!current.isAsciiDigit()) {
            return false;
        }
    } else if (!current.isAsciiDigit()) {
        return true;
    }
    if (!readNDigits(unit::CpLength{2U}, _second)) {
        return false;
    }
    _precision = DateTimePrecision::Second;
    if (_reader.advanceIf(fractionSeparator)) {
        return parseFraction();
    }
    return true;
}

auto IsoDateTimeParser::parseFraction() noexcept -> bool {
    constexpr auto options = text::IntegerParseOptions::parserDefault()
                                 .setFixedBase(text::IntegerBase::Decimal)
                                 .setMaximumDigits(unit::CpLength{9});
    const auto result = _reader.parseInteger(options);
    using St = text::ReadNumberStatus;
    if (result.status != St::Success) {
        return false;
    }
    _nanosecond = static_cast<int>(result.value); // parsed range is 0-999'999'999, always fit signed 32bit.
    for (auto i = result.digitCount; i < unit::CpLength{9}; ++i) {
        _nanosecond *= 10;
    }
    _precision = DateTimePrecision::Nanosecond;
    return true;
}

auto IsoDateTimeParser::parseOffset() noexcept -> bool {
    if (_reader.advanceIf(U'Z')) {
        _hasOffset = true;
        _offset = Seconds{};
        return true;
    }
    auto sign = 1;
    if (_reader.advanceIf(U'-')) {
        sign = -1;
    } else if (!_reader.advanceIf(U'+')) {
        return false;
    }
    auto hour = 0;
    auto minute = 0;
    auto second = 0;
    if (!readNDigits(unit::CpLength{2U}, hour)) {
        return false;
    }
    const auto extended = _reader.advanceIf(U':');
    if (extended) {
        if (!readNDigits(unit::CpLength{2U}, minute)) {
            return false;
        }
        if (_reader.advanceIf(U':') && !readNDigits(unit::CpLength{2U}, second)) {
            return false;
        }
    } else if (_reader.peek().isAsciiDigit() && !readNDigits(unit::CpLength{2U}, minute)) {
        return false;
    }
    if (hour > 23 || minute > 59 || second > 59) {
        return false;
    }
    _hasOffset = true;
    _offset = Seconds{sign * (hour * 3600 + minute * 60 + second)};
    return true;
}

auto IsoDateTimeParser::readNDigits(const unit::CpLength count, int &value) noexcept -> bool {
    value = 0;
    auto options = text::IntegerParseOptions::fixedDecimal(count);
    const auto result = _reader.parseInteger(options);
    if (result.status != text::ReadNumberStatus::Success) {
        return false;
    }
    value = static_cast<int>(result.value);
    return true;
}

}
