// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Duration.hpp"

#include "TimeDelta.hpp"

#include "../err/OverflowError.hpp"

namespace erbsland::time {

namespace {
constexpr auto cSecondsPerDay = 86400.0;
}

Duration::Duration(Parts parts) noexcept :
    _seconds{
        parts.seconds + parts.minutes.converted<Seconds>() + parts.hours.converted<Seconds>() +
        parts.days.converted<Seconds>() + parts.weeks.converted<Seconds>()} {
}

auto Duration::operator+(Duration other) const noexcept -> Duration {
    return Duration{_seconds + other._seconds};
}

auto Duration::operator+=(Duration other) noexcept -> Duration & {
    _seconds += other._seconds;
    return *this;
}

auto Duration::operator-(Duration other) const noexcept -> Duration {
    return Duration{_seconds - other._seconds};
}

auto Duration::operator-=(Duration other) noexcept -> Duration & {
    _seconds -= other._seconds;
    return *this;
}

auto Duration::operator-() const noexcept -> Duration {
    return Duration{-_seconds};
}

auto Duration::seconds() const noexcept -> Seconds {
    return _seconds % 60;
}

auto Duration::minutes() const noexcept -> Minutes {
    return _seconds.converted<Minutes>() % 60;
}

auto Duration::hours() const noexcept -> Hours {
    return _seconds.converted<Hours>() % 24;
}

auto Duration::days() const noexcept -> Days {
    return _seconds.converted<Days>();
}

auto Duration::parts(DurationPart largestPart) const noexcept -> Parts {
    auto result = Parts{};
    if (largestPart == DurationPart::Seconds) {
        result.seconds = _seconds;
        return result;
    }

    auto secondsRemainder = _seconds;
    result.minutes = secondsRemainder.extract<Minutes>();
    result.seconds = secondsRemainder;
    if (largestPart == DurationPart::Minutes) {
        return result;
    }

    auto minutesRemainder = result.minutes;
    result.hours = minutesRemainder.extract<Hours>();
    result.minutes = minutesRemainder;
    if (largestPart == DurationPart::Hours) {
        return result;
    }

    auto hoursRemainder = result.hours;
    result.days = hoursRemainder.extract<Days>();
    result.hours = hoursRemainder;
    if (largestPart == DurationPart::Weeks) {
        result.weeks = result.days.extract<Weeks>();
        return result;
    }
    return result;
}

auto Duration::toStdSeconds() const noexcept -> std::chrono::seconds {
    return std::chrono::seconds{_seconds.toValue().cast<std::chrono::seconds::rep>().toRawValue()};
}

auto Duration::toDaysAndNanoseconds() const noexcept -> DaysAndNanoseconds {
    auto seconds = _seconds;
    const auto days = seconds.extract<Days>();
    return {.days = days, .nanoseconds = seconds.converted<Nanoseconds>()};
}

auto Duration::toDaysWithFractions() const noexcept -> double {
    return static_cast<double>(_seconds.toValue().toRawValue()) / cSecondsPerDay;
}

auto Duration::wouldConvertToTimeDeltaSaturate() const noexcept -> bool {
    return _seconds.wouldConvertSaturate<Nanoseconds>();
}

auto Duration::toTimeDelta() const noexcept -> TimeDelta {
    return TimeDelta{_seconds.converted<Nanoseconds>()};
}

auto Duration::toTimeDeltaOrThrow() const -> TimeDelta {
    if (wouldConvertToTimeDeltaSaturate()) {
        throw err::OverflowError{"Duration conversion to TimeDelta would exceed nanosecond bounds"};
    }
    return TimeDelta{_seconds.convertedOrThrow<Nanoseconds>()};
}

}
