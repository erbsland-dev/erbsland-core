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
#include "../text/StringBuilder.hpp"
#include "../unit/CpLength.hpp"

#include <chrono>

namespace erbsland::time {

namespace {
constexpr auto cLastValidSecond = Seconds{315569519999};
}

using namespace text::literals;

DateTime::DateTime(const Date localDate, const Time localTime, const Seconds offset) noexcept :
    _date{localDate}, _time{localTime}, _offset{tz::TimeOffset{offset}} {
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

auto DateTime::timeZoneAbbreviation() const -> text::String {
    return TimeZone::abbreviation(_offset);
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
        return days.isNegative() ? first() : last();
    }
    const auto newDate = _date.added(days);
    auto result = DateTime{newDate, newTime};
    if (_offset.isZone()) {
        return result.toTimeZone(timeZone());
    }
    if (_offset.isStaticOffset()) {
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

auto DateTime::toIsoString(IsoTimeFormatFlags flags, DateTimePrecision precision) const -> text::String {
    if (!isValid()) {
        return {};
    }
    const auto datePrecision = precision < DateTimePrecision::Day ? precision : DateTimePrecision::Day;
    auto result = date().toIsoString(flags, datePrecision);
    if (precision >= DateTimePrecision::Hour) {
        result.append(flags.isSet(IsoTimeFormat::TimePrefix) ? "T"_el : " "_el);
        auto timeFlags = flags;
        timeFlags.clear(IsoTimeFormat::TimePrefix);
        result.append(time().toIsoString(timeFlags, precision));
        if (flags.isSet(IsoTimeFormat::TimeShift)) {
            result.append(isoTimeShiftString(_offset.offset(), flags));
        }
    }
    return result;
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

auto DateTime::fromIsoString(text::StringView text, DateTimePrecision requiredPrecision) noexcept -> DateTime {
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

auto DateTime::fromIsoStringOrThrow(text::StringView text, DateTimePrecision requiredPrecision) -> DateTime {
    auto result = fromIsoString(text, requiredPrecision);
    if (!result.isValid()) {
        throw err::ParseError{"Invalid ISO date/time string"};
    }
    return result;
}

auto DateTime::fromIsoString(text::StringView text, TimeZone timeZone, DateTimePrecision requiredPrecision) noexcept
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

auto DateTime::fromIsoStringOrThrow(text::StringView text, TimeZone timeZone, DateTimePrecision requiredPrecision)
    -> DateTime {
    auto result = fromIsoString(text, timeZone, requiredPrecision);
    if (!result.isValid()) {
        throw err::ParseError{"Invalid ISO local date/time string"};
    }
    return result;
}

auto DateTime::isoTimeShiftString(Seconds offset, IsoTimeFormatFlags flags) -> text::String {
    if (offset.isZero() && !flags.isSet(IsoTimeFormat::TimeShiftAlwaysComplete)) {
        return text::String{"Z"_el};
    }

    auto digitFormat = text::IntegerFormat::decimal();
    digitFormat.addFlags(text::IntegerFormatFlag::ZeroFill).setFieldWidth(unit::CpLength{2U});

    const auto sign = offset.isNegative() ? U'-' : U'+';
    auto remainingOffset = offset.isNegative() ? -offset : offset;
    const auto hours = remainingOffset.extract<Hours>();
    const auto minutes = remainingOffset.extract<Minutes>();
    const auto seconds = remainingOffset;
    const auto extended = flags.isSet(IsoTimeFormat::Extended);
    const auto includeSeconds = flags.isSet(IsoTimeFormat::TimeShiftUpToSeconds) &&
        (!seconds.isZero() || flags.isSet(IsoTimeFormat::TimeShiftAlwaysComplete));

    auto builder = text::StringBuilder{};
    builder.append(sign);
    builder.appendInteger(hours.toValue(), digitFormat);
    if (extended) {
        builder.append(U':');
    }
    builder.appendInteger(minutes.toValue(), digitFormat);
    if (includeSeconds) {
        if (extended) {
            builder.append(U':');
        }
        builder.appendInteger(seconds.toValue(), digitFormat);
    }
    return builder.takeU8String();
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
