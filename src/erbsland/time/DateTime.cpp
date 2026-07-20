// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DateTime.hpp"

#include "impl/IsoDateTimeParser.hpp"
#include "impl/PosixTimeConverter.hpp"
#include "impl/WindowsTimeConverter.hpp"

#include "../err/OverflowError.hpp"
#include "../err/ParseError.hpp"
#include "../text/IntegerFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/CpLength.hpp"

#include <chrono>

namespace erbsland::time {

using text::IntegerFormat;
using text::IntegerFormatFlag;
using text::String;
using text::StringEditor;

namespace {
constexpr auto cLastValidSecond = Seconds{315569519999};
}

using namespace text::literals;

DateTime::DateTime(const Date localDate, const Time localTime, const Seconds offset) noexcept :
    _date{localDate}, _time{localTime}, _offset{tz::TimeOffset{offset}} {
    subtractOffset();
}

DateTime::DateTime(const Date localDate, const TimeWithZone localTime, const TimeOccurrenceInFold occurrence) noexcept :
    _date{localDate},
    _time{localTime.time()},
    _offset{localTime.timeZone().timeOffsetAtLocal(localDate, localTime.time(), occurrence)} {
    subtractOffset();
}

DateTime::DateTime(
    const Date localDate, const Time localTime, const TimeZone timeZone, const TimeOccurrenceInFold occurrence) noexcept
    :
    _date{localDate}, _time{localTime}, _offset{timeZone.timeOffsetAtLocal(localDate, localTime, occurrence)} {
    subtractOffset();
}

auto DateTime::operator<=>(const DateTime &other) const noexcept -> std::strong_ordering {
    if (const auto result = _date <=> other._date; result != std::strong_ordering::equal) {
        return result;
    }
    return _time <=> other._time;
}

auto DateTime::parts() const noexcept -> DateTimeParts {
    const auto localDate = date();
    const auto localTime = time();
    return {
        .year = localDate.year(),
        .month = localDate.month(),
        .day = localDate.day(),
        .hour = localTime.hour(),
        .minute = localTime.minute(),
        .second = localTime.second(),
        .nanosecondFraction = localTime.nanosecondFraction()};
}

auto DateTime::timeZoneAbbreviation() const -> String {
    return TimeZone::abbreviation(_offset);
}

auto DateTime::timeZone() const noexcept -> TimeZone {
    auto result = TimeZone{_offset.zoneId()};
    if (_offset.isStaticOffset()) {
        result = TimeZone{Duration{_offset.offset()}};
    }
    result._isLocalTime = _offset.isLocalTime();
    return result;
}

auto DateTime::wouldAddSaturate(const Duration duration) const noexcept -> bool {
    if (!isValid()) {
        return false;
    }
    auto time = _time;
    const auto days = time.addWithWrap(duration);
    return _date.wouldAddSaturate(days);
}

auto DateTime::wouldSubtractSaturate(const Duration duration) const noexcept -> bool {
    return wouldAddSaturate(-duration);
}

auto DateTime::added(const Duration duration) const noexcept -> DateTime {
    if (!isValid()) {
        return {};
    }
    auto newTime = _time;
    const auto days = newTime.addWithWrap(duration);
    if (_date.wouldAddSaturate(days)) {
        auto result = days.isNegative() ? first() : last();
        if (_offset.isZone()) {
            return result.toTimeZone(timeZone());
        }
        result._offset = _offset;
        return result;
    }
    const auto newDate = _date.added(days);
    auto result = DateTime{newDate, newTime};
    if (_offset.isZone()) {
        return result.toTimeZone(timeZone());
    }
    if (_offset.isStaticOffset() || _offset.isLocalTime()) {
        result._offset = _offset;
    }
    return result;
}

auto DateTime::addedOrThrow(const Duration duration) const -> DateTime {
    if (wouldAddSaturate(duration)) {
        throw err::OverflowError{"DateTime addition would exceed supported date/time range"};
    }
    return added(duration);
}

auto DateTime::subtractedOrThrow(const Duration duration) const -> DateTime {
    if (wouldSubtractSaturate(duration)) {
        throw err::OverflowError{"DateTime subtraction would exceed supported date/time range"};
    }
    return subtracted(duration);
}

auto DateTime::durationTo(const DateTime &other) const noexcept -> Duration {
    return Duration{other.toSecondsSinceEpoch() - toSecondsSinceEpoch()};
}

auto DateTime::timeDeltaTo(const DateTime &other) const noexcept -> TimeDelta {
    return TimeDelta{durationTo(other).toSeconds()};
}

auto DateTime::toUtc() const noexcept -> DateTime {
    return isValid() ? DateTime{_date, _time} : DateTime{};
}

auto DateTime::toString() const -> String {
    if (!isValid()) {
        return {};
    }
    return String::fromJoined(
        {date().toString(),
            " "_el,
            time().toString(),
            _offset.isLocalTime()
                ? String{}
                : isoTimeShiftString(
                      _offset.offset(),
                      IsoTimeFormatFlags{IsoTimeFormat::Extended, IsoTimeFormat::TimeShiftUpToSeconds})});
}

auto DateTime::toTimeZone(TimeZone timeZone) const noexcept -> DateTime {
    if (!isValid()) {
        return {};
    }
    return DateTime{_date, _time, timeZone.timeOffsetAtUtc(_date, _time), PrivateTag{}};
}

auto DateTime::toSecondsSinceEpoch() const noexcept -> Seconds {
    if (!isValid()) {
        return Seconds{-1};
    }
    return _date.toDaysSinceEpoch().converted<Seconds>() + _time.toSecondsSinceMidnight();
}

auto DateTime::toTimeT() const noexcept -> std::time_t {
    return impl::PosixTimeConverter::toTimeT(*this);
}

auto DateTime::toWindowsFileTimeTicks() const noexcept -> std::optional<std::uint64_t> {
    return impl::WindowsTimeConverter::toFileTimeTicks(*this);
}

auto DateTime::toIsoString(IsoTimeFormatFlags flags, DateTimePrecision precision) const -> String {
    if (!isValid()) {
        return {};
    }
    const auto datePrecision = precision < DateTimePrecision::Day ? precision : DateTimePrecision::Day;
    const auto dateText = date().toIsoString(flags, datePrecision);
    if (precision < DateTimePrecision::Hour) {
        return dateText;
    }
    auto timeFlags = flags;
    timeFlags.clear(IsoTimeFormat::TimePrefix);
    return String::fromJoined(
        {dateText,
            flags.isSet(IsoTimeFormat::TimePrefix) ? String{"T"_el} : String{" "_el},
            time().toIsoString(timeFlags, precision),
            flags.isSet(IsoTimeFormat::TimeShift) ? isoTimeShiftString(_offset.offset(), flags) : String{}});
}

auto DateTime::now() noexcept -> DateTime {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() - seconds * 1000000000LL;
    return fromSecondsSinceEpoch(posixEpochSecondsDelta() + Seconds{seconds}, Nanoseconds{ns});
}

auto DateTime::fromSecondsSinceEpoch(Seconds seconds, Nanoseconds fractions) noexcept -> DateTime {
    if (seconds < Seconds::zero() || seconds > cLastValidSecond) {
        return {};
    }
    auto secondOfDay = seconds;
    const auto days = secondOfDay.extract<Days>();
    return DateTime{
        Date::fromDaysSinceEpoch(days), Time::fromDurationSinceMidnight(TimeDelta{secondOfDay} + TimeDelta{fractions})};
}

auto DateTime::fromTimeT(std::time_t posixTime) noexcept -> DateTime {
    return impl::PosixTimeConverter::fromTimeT(posixTime);
}

auto DateTime::fromPosixTime(const Seconds seconds, const Nanoseconds fractions) noexcept -> DateTime {
    return impl::PosixTimeConverter::fromPosixTime(seconds, fractions);
}

auto DateTime::fromWindowsFileTimeTicks(const std::uint64_t ticks) noexcept -> DateTime {
    return impl::WindowsTimeConverter::fromFileTimeTicks(ticks);
}

auto DateTime::fromIsoString(const String &text, DateTimePrecision requiredPrecision) noexcept -> DateTime {
    const auto parsed = impl::IsoDateTimeParser::parse(text, true);
    if (!parsed.has_value()) {
        return {};
    }
    if (parsed->precision < requiredPrecision) {
        return {};
    }
    if (parsed->hasOffset) {
        return DateTime{parsed->date, parsed->time, parsed->offset};
    }
    return DateTime{parsed->date, parsed->time};
}

auto DateTime::fromIsoStringOrThrow(const String &text, DateTimePrecision requiredPrecision) -> DateTime {
    auto result = fromIsoString(text, requiredPrecision);
    if (!result.isValid()) {
        throw err::ParseError{"Invalid ISO date/time string"};
    }
    return result;
}

auto DateTime::fromIsoString(const String &text, TimeZone timeZone, DateTimePrecision requiredPrecision) noexcept
    -> DateTime {
    const auto parsed = impl::IsoDateTimeParser::parse(text, false);
    if (!parsed.has_value()) {
        return {};
    }
    if (parsed->precision < requiredPrecision) {
        return {};
    }
    return DateTime{parsed->date, parsed->time, timeZone};
}

auto DateTime::fromIsoStringOrThrow(const String &text, TimeZone timeZone, DateTimePrecision requiredPrecision)
    -> DateTime {
    auto result = fromIsoString(text, timeZone, requiredPrecision);
    if (!result.isValid()) {
        throw err::ParseError{"Invalid ISO local date/time string"};
    }
    return result;
}

auto DateTime::isoTimeShiftString(Seconds offset, IsoTimeFormatFlags flags) -> String {
    if (offset.isZero() && !flags.isSet(IsoTimeFormat::TimeShiftAlwaysComplete)) {
        return "Z"_el;
    }

    auto digitFormat = IntegerFormat::decimal();
    digitFormat.addFlags(IntegerFormatFlag::ZeroFill).setFieldWidth(unit::CpLength{2U});

    const auto sign = offset.isNegative() ? U'-' : U'+';
    auto remainingOffset = offset.isNegative() ? -offset : offset;
    const auto hours = remainingOffset.extract<Hours>();
    const auto minutes = remainingOffset.extract<Minutes>();
    const auto seconds = remainingOffset;
    const auto extended = flags.isSet(IsoTimeFormat::Extended);
    const auto includeSeconds = flags.isSet(IsoTimeFormat::TimeShiftUpToSeconds) &&
        (!seconds.isZero() || flags.isSet(IsoTimeFormat::TimeShiftAlwaysComplete));

    const auto separator = extended ? String{":"_el} : String{};
    return String::fromJoined(
        {String::fromCharacter(sign),
            String::fromInteger(hours.toValue(), digitFormat),
            separator,
            String::fromInteger(minutes.toValue(), digitFormat),
            includeSeconds ? separator : String{},
            includeSeconds ? String::fromInteger(seconds.toValue(), digitFormat) : String{}});
}

auto DateTime::posixEpochSecondsDelta() noexcept -> Seconds {
    return Days{719528}.converted<Seconds>();
}

auto DateTime::posixEpoch() noexcept -> DateTime {
    return DateTime{Date::fromDaysSinceEpoch(Days{719528}), Time{}};
}

auto DateTime::localDateTime() const noexcept -> std::pair<Date, Time> {
    if (!isValid()) {
        return {{}, {}};
    }
    auto time = _time;
    auto date = _date;
    const auto days = time.addWithWrap(Duration{_offset.offset()});
    date.add(days);
    return {date, time};
}

void DateTime::subtractOffset() noexcept {
    if (!isValid()) {
        _time = {};
        _offset = {};
        return;
    }
    const auto days = _time.addWithWrap(Duration{-_offset.offset()});
    _date.add(days);
    if (!_date.isValid()) {
        _time = {};
        _offset = {};
    }
}

}
