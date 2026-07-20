// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueDateTime.hpp"

#include "Number.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

using ParsedTime = std::variant<time::Time, time::TimeWithZone>;

auto scanTimeValue(TokenDecoder &decoder) -> std::optional<ParsedTime>;
auto scanDate(TokenDecoder &decoder) -> std::optional<time::Date>;

auto scanDateOrDateTime(TokenDecoder &decoder) -> std::optional<LexerToken> {
    if (decoder.character() != CharClass::DecimalDigit) {
        return std::nullopt;
    }
    auto dateTimeTransaction = Transaction{decoder};
    auto optDate = scanDate(decoder);
    if (!optDate) {
        return std::nullopt;
    }
    auto timeSeparatorTransaction = Transaction{decoder};
    if (decoder.character() == nc::space || decoder.character() == CharClass::LetterT) {
        const bool hasLetterSeparator = decoder.character() == CharClass::LetterT;
        decoder.next();
        if (decoder.character() == CharClass::DecimalDigit) {
            if (auto optTime = scanTimeValue(decoder)) {
                timeSeparatorTransaction.commit();
                dateTimeTransaction.commit();
                const auto date = std::move(optDate).value();
                const auto dateTime = std::visit(
                    [date](const auto &parsedTime) -> time::DateTime {
                        using Value = std::remove_cvref_t<decltype(parsedTime)>;
                        if constexpr (std::is_same_v<Value, time::Time>) {
                            return time::DateTime{date, parsedTime, time::TimeZone::local()};
                        } else {
                            return time::DateTime{date, parsedTime};
                        }
                    },
                    *optTime);
                return decoder.createToken(TokenType::DateTime, dateTime);
            }
        } else if (hasLetterSeparator) {
            decoder.throwSyntaxOrUnexpectedEndError("Expected a time value after a time separator."_el);
        }
    }
    timeSeparatorTransaction.rollback(); // only rollback the inner scope.
    dateTimeTransaction.commit();        // only commit the outer scope.
    return decoder.createToken(TokenType::Date, std::move(optDate).value());
}

auto scanTime(TokenDecoder &decoder) -> std::optional<LexerToken> {
    if (decoder.character() != CharClass::TimeStart) {
        return {};
    }
    auto transaction = Transaction{decoder};
    auto optTime = scanTimeValue(decoder);
    if (!optTime) {
        return {};
    }
    transaction.commit();
    return std::visit(
        [&decoder](auto &&value) -> LexerToken {
            return decoder.createToken(TokenType::Time, std::forward<decltype(value)>(value));
        },
        std::move(optTime).value());
}

/// Parse a time value.
/// @return If the value does not start with `XX:`, no value is returned instead of an error.
/// @throws ConfError If there is a syntax error after the initial `XX:`.
auto scanTimeValue(TokenDecoder &decoder) -> std::optional<ParsedTime> {
    if (decoder.character() == CharClass::LetterT) {
        decoder.next(); // Skip an optional T in front of the time.
        if (decoder.character() != CharClass::DecimalDigit) {
            return {};  // coverage: this case is already handled by the value-literal scan.
        }
    }
    int64_t hour = 0;
    int64_t minute = 0;
    int64_t second = 0;
    int64_t fraction = 0;
    auto offsetSign = Sign::Positive;
    int64_t offsetHour = -1;
    int64_t offsetMinute = 0;
    parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2).assignTo(hour);
    if (hour < 0 || decoder.character() != nc::timeSeparator) {
        return {};
    }
    decoder.next(); // From here, we expect to read a time and throw an error if the format does not match our
                    // expectations.
    if (hour > 23) {
        decoder.throwSyntaxError("The hour in a time value must be in the range 00-23."_el);
    }
    if (decoder.character() != CharClass::DecimalDigit) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected the minute part after the colon for a time value."_el);
    }
    parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2).assignTo(minute);
    if (minute < 0) {
        decoder.throwSyntaxOrUnexpectedEndError(
            "Expected a two digit minute part after the colon for a time value."_el);
    }
    if (minute > 59) {
        decoder.throwSyntaxError("The minute in a time value must be in the range 00-59."_el);
    }
    // the second part is optional.
    if (decoder.character() == nc::timeSeparator) {
        decoder.next();
        if (decoder.character() != CharClass::DecimalDigit) {
            decoder.throwSyntaxOrUnexpectedEndError(
                "Expected the second part after the second colon for a time value."_el);
        }
        parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2).assignTo(second);
        if (second < 0) {
            decoder.throwSyntaxOrUnexpectedEndError(
                "Expected a two digit second part after the second colon for a time value."_el);
        }
        if (second > 59) {
            decoder.throwSyntaxError("The second in a time value must be in the range 00-59."_el);
        }
        // optional second fraction.
        if (decoder.character() == nc::decimalPoint) {
            decoder.next();
            if (decoder.character() != CharClass::DecimalDigit) {
                decoder.throwSyntaxOrUnexpectedEndError(
                    "Expected the second fraction part after the decimal point."_el);
            }
            std::size_t digitCount = 0;
            parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No)
                .assignTo(fraction, digitCount);
            if (fraction < 0) {
                decoder.throwSyntaxOrUnexpectedEndError("Expected a fraction part after the decimal point."_el);
            }
            if (digitCount > 9) {
                decoder.throwSyntaxError("The fraction part in a time must not exceed nine digits."_el);
            }
            constexpr std::size_t maxFractionDigits = 9;
            for (std::size_t i = 0; i < maxFractionDigits - digitCount; ++i) {
                fraction *= 10; // Shift the fraction to nanoseconds.
            }
        }
    }
    // may be followed by an offset.
    if (decoder.character() == CharClass::LetterZ) {
        decoder.next(); // consume the Z.
        offsetHour = 0;
    } else if (decoder.character() == CharClass::PlusOrMinus) {
        offsetSign = (decoder.character() == nc::minus ? Sign::Negative : Sign::Positive);
        decoder.next();
        if (decoder.character() != CharClass::DecimalDigit) {
            decoder.throwSyntaxOrUnexpectedEndError("Expected an offset hour."_el);
        }
        parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2).assignTo(offsetHour);
        if (offsetHour < 0) {
            decoder.throwSyntaxOrUnexpectedEndError("Expected a two digit offset hour."_el);
        }
        if (offsetHour > 23) {
            decoder.throwSyntaxError("The offset hour must be in the range 00-23."_el);
        }
        if (decoder.character() == nc::timeSeparator) {
            decoder.next();
            if (decoder.character() != CharClass::DecimalDigit) {
                decoder.throwSyntaxOrUnexpectedEndError("Expected an offset minute."_el);
            }
            parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2)
                .assignTo(offsetMinute);
            if (offsetMinute < 0) {
                decoder.throwSyntaxOrUnexpectedEndError("Expected a two digit offset minute."_el);
            }
            if (offsetMinute > 59) {
                decoder.throwSyntaxError("The offset minute must be in the range 00-59."_el);
            }
        }
    }
    const auto parsedTime =
        time::Time{time::Hour{hour}, time::Minute{minute}, time::Second{second}, time::Nanoseconds{fraction}};
    if (offsetHour < 0) {
        return ParsedTime{parsedTime};
    }
    auto offsetSeconds = (offsetHour * 60 + offsetMinute) * 60;
    if (offsetSign == Sign::Negative) {
        offsetSeconds = -offsetSeconds;
    }
    return ParsedTime{time::TimeWithZone{parsedTime, time::TimeZone{time::Duration{time::Seconds{offsetSeconds}}}}};
}

/// Parse a date value.
/// @return If the value does not start with `XXXX-`, no value is returned instead of an error.
/// @throws ConfError If there is a syntax error after the initial `XXXX-`.
auto scanDate(TokenDecoder &decoder) -> std::optional<time::Date> {
    int64_t year = 0;
    int64_t month = 0;
    int64_t day = 0;
    parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 4).assignTo(year);
    if (year < 0) {
        return {};
    }
    if (decoder.character() != nc::dateSeparator) {
        return {};
    }
    decoder.next();
    if (decoder.character() != CharClass::DecimalDigit) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected a month part after the date separator."_el);
    }
    parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2).assignTo(month);
    if (month < 0) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected two digits for the month in a date."_el);
    }
    if (month < 1 || month > 12) {
        decoder.throwSyntaxError("The month in a date value must be in the range 01-12."_el);
    }
    if (decoder.character() != nc::dateSeparator) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected a date separator after the month."_el);
    }
    decoder.next();
    if (decoder.character() != CharClass::DecimalDigit) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected a day part after the date separator."_el);
    }
    parseNumber(decoder, text::IntegerBase::Decimal, Sign::Positive, NumberSeparators::No, 2).assignTo(day);
    if (day < 0) {
        decoder.throwSyntaxOrUnexpectedEndError("Expected two digits for the day in a date."_el);
    }
    if (day < 1 || day > 31) {
        decoder.throwSyntaxError("The day in a date value must be in the range 01-31."_el);
    }
    if (year < 1 || year > 9999) {
        decoder.throwSyntaxError("The year in a date value must be in the range 0001-9999."_el);
    }
    if (!time::Date::exists(time::Year{year}, time::Month{month}, time::Day{day})) {
        decoder.throwSyntaxError("This date does not exist."_el);
    }
    return time::Date::fromYearMonthDay(static_cast<int>(year), static_cast<int>(month), static_cast<int>(day));
}

}
