// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Time.hpp"

#include "../text/IntegerFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringBuilder.hpp"
#include "../unit/CpLength.hpp"

namespace erbsland::time {

namespace {
constexpr auto cNanosecondsPerSecond = int64_t{1000000000};
constexpr auto cSecondsPerDay = int64_t{86400};
constexpr auto cNanosecondsPerDay = cNanosecondsPerSecond * cSecondsPerDay;
}

using namespace text::literals;

Time::Time(const Hour hour, const Minute minute, const Second second, const Nanoseconds nsFraction) noexcept :
    _nanoseconds{
        (((hour.toAmount().toValue() * 60 + minute.toAmount().toValue()) * 60 + second.toAmount().toValue()) *
            cNanosecondsPerSecond) +
        nsFraction.toValue().clamped(0LL, cNanosecondsPerSecond - 1LL)} {
}

auto Time::hour() const noexcept -> Hour {
    return Hour{_nanoseconds / cNanosecondsPerSecond / 3600LL};
}

auto Time::minute() const noexcept -> Minute {
    return Minute{(_nanoseconds / cNanosecondsPerSecond / 60LL) % 60LL};
}

auto Time::second() const noexcept -> Second {
    return Second{(_nanoseconds / cNanosecondsPerSecond) % 60LL};
}

auto Time::millisecondFraction() const noexcept -> Milliseconds {
    return Nanoseconds{_nanoseconds % cNanosecondsPerSecond}.converted<Milliseconds>();
}

auto Time::nanosecondFraction() const noexcept -> Nanoseconds {
    return Nanoseconds{_nanoseconds % cNanosecondsPerSecond};
}

auto Time::parts() const noexcept -> TimeParts {
    return {.hour = hour(), .minute = minute(), .second = second(), .nanosecondFraction = nanosecondFraction()};
}

auto Time::durationSinceMidnight() const noexcept -> Duration {
    return Duration{Seconds{_nanoseconds / cNanosecondsPerSecond}};
}

auto Time::timeDeltaSinceMidnight() const noexcept -> TimeDelta {
    return TimeDelta{Nanoseconds{_nanoseconds}};
}

auto Time::toSecondsSinceMidnight() const noexcept -> Seconds {
    return Seconds{_nanoseconds / cNanosecondsPerSecond};
}

auto Time::toNanosecondsSinceMidnight() const noexcept -> Nanoseconds {
    return Nanoseconds{_nanoseconds};
}

auto Time::toIsoString(const IsoTimeFormatFlags flags, const DateTimePrecision precision) const -> text::String {
    const auto extended = flags.isSet(IsoTimeFormat::Extended);
    const auto separator = flags.isSet(IsoTimeFormat::UseDotFraction) ? U'.' : U',';
    auto twoDigitFormat = text::IntegerFormat::decimal();
    twoDigitFormat.addFlags(text::IntegerFormatFlag::ZeroFill).setFieldWidth(unit::CpLength{2U});

    auto builder = text::StringBuilder{};
    if (flags.isSet(IsoTimeFormat::TimePrefix)) {
        builder.append("T"_el);
    }
    builder.appendInteger(hour().toValue(), twoDigitFormat);
    if (precision >= DateTimePrecision::Minute) {
        if (extended) {
            builder.append(U':');
        }
        builder.appendInteger(minute().toValue(), twoDigitFormat);
    }
    if (precision >= DateTimePrecision::Second) {
        if (extended) {
            builder.append(U':');
        }
        builder.appendInteger(second().toValue(), twoDigitFormat);
    }
    if (precision >= DateTimePrecision::Millisecond) {
        auto fractionFormat = text::IntegerFormat::decimal();
        auto fraction = nanosecondFraction().toValue();
        auto fieldWidth = unit::CpLength{3};
        if (precision == DateTimePrecision::Microsecond) {
            fraction /= 1000LL; // microsecond fraction
            fieldWidth = unit::CpLength{6};
        } else if (precision == DateTimePrecision::Nanosecond) {
            fieldWidth = unit::CpLength{9}; // full nanoseconds
        } else {
            fraction /= 1000000LL;          // millisecond fraction
        }
        fractionFormat.addFlags(text::IntegerFormatFlag::ZeroFill).setFieldWidth(fieldWidth);
        builder.append(separator);
        builder.appendInteger(fraction, fractionFormat);
    }
    return builder.takeU8String();
}

auto Time::addWithWrap(const TimeDelta delta) noexcept -> Days {
    _nanoseconds.add(delta.toNanoseconds().toValue());
    return Days{_nanoseconds.wrapAndCount(0LL, cNanosecondsPerDay - 1LL)};
}

auto Time::addWithWrap(const Duration duration) noexcept -> Days {
    const auto [days, nanoseconds] = duration.toDaysAndNanoseconds();
    return days + addWithWrap(TimeDelta{nanoseconds});
}

auto Time::addedWithWrap(const TimeDelta delta) const noexcept -> TimeWrapResult {
    auto result = *this;
    auto days = result.addWithWrap(delta);
    return {.time = result, .days = days};
}

auto Time::addedWithWrap(Duration duration) const noexcept -> TimeWrapResult {
    auto result = *this;
    auto days = result.addWithWrap(duration);
    return {.time = result, .days = days};
}

auto Time::fromDurationSinceMidnight(const TimeDelta duration) noexcept -> Time {
    auto result = Time{};
    result.addWithWrap(duration);
    return result;
}

auto Time::fromDurationSinceMidnight(Duration duration) noexcept -> Time {
    auto result = Time{};
    result.addWithWrap(duration);
    return result;
}

auto Time::first() noexcept -> Time {
    return Time{Hour{0}, Minute{0}, Second{0}, Nanoseconds{0}};
}

auto Time::last() noexcept -> Time {
    return Time{Hour{23}, Minute{59}, Second{59}, Nanoseconds{999999999}};
}

}
