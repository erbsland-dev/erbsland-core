// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeDelta.hpp"

#include "Duration.hpp"

#include "impl/TimeDeltaFormatter.hpp"

#include "../err/OverflowError.hpp"

namespace erbsland::time {

using namespace text::literals;

namespace {
constexpr auto cNanosecondsPerSecondFloat = 1000000000.0;
constexpr auto cNanosecondsPerDayFloat = 86400.0 * cNanosecondsPerSecondFloat;
}

template <typename tTimeUnit>
auto TimeDelta::createOrThrow(const tTimeUnit value) -> TimeDelta {
    if (value.template wouldConvertSaturate<Nanoseconds>()) {
        throw err::OverflowError{"Exceeds the maximum representable time-delta value."_el};
    }
    return TimeDelta{value};
}

auto TimeDelta::operator+(TimeDelta other) const noexcept -> TimeDelta {
    return TimeDelta{_nanoseconds + other._nanoseconds};
}

auto TimeDelta::operator+=(TimeDelta other) noexcept -> TimeDelta & {
    _nanoseconds += other._nanoseconds;
    return *this;
}

auto TimeDelta::operator-(TimeDelta other) const noexcept -> TimeDelta {
    return TimeDelta{_nanoseconds - other._nanoseconds};
}

auto TimeDelta::operator-=(TimeDelta other) noexcept -> TimeDelta & {
    _nanoseconds -= other._nanoseconds;
    return *this;
}

auto TimeDelta::operator-() const noexcept -> TimeDelta {
    return TimeDelta{-_nanoseconds};
}

auto TimeDelta::operator/(const IntegerValue divisor) const noexcept -> TimeDelta {
    return TimeDelta{_nanoseconds / divisor};
}

auto TimeDelta::operator/=(IntegerValue divisor) noexcept -> TimeDelta & {
    _nanoseconds /= divisor;
    return *this;
}

auto TimeDelta::operator/(const TimeDelta divisor) const noexcept -> IntegerValue {
    return _nanoseconds.toValue() / divisor._nanoseconds.toValue();
}

auto TimeDelta::operator*(IntegerValue factor) const noexcept -> TimeDelta {
    return TimeDelta{_nanoseconds * factor};
}

auto TimeDelta::operator*=(IntegerValue factor) noexcept -> TimeDelta & {
    _nanoseconds *= factor;
    return *this;
}

auto TimeDelta::toMilliseconds() const noexcept -> Milliseconds {
    return _nanoseconds.converted<Milliseconds>();
}

auto TimeDelta::toSeconds() const noexcept -> Seconds {
    return _nanoseconds.converted<Seconds>();
}

auto TimeDelta::toString(const TimeDeltaFormat &format) const -> text::String {
    return impl::formatTimeDelta(_nanoseconds, format);
}

auto TimeDelta::toSecondsWithFractions() const noexcept -> double {
    return static_cast<double>(_nanoseconds.toValue().toRawValue()) / cNanosecondsPerSecondFloat;
}

auto TimeDelta::toDaysWithFractions() const noexcept -> double {
    return static_cast<double>(_nanoseconds.toValue().toRawValue()) / cNanosecondsPerDayFloat;
}

auto TimeDelta::toStdNanoseconds() const noexcept -> std::chrono::nanoseconds {
    return std::chrono::nanoseconds{_nanoseconds.toValue().cast<std::chrono::nanoseconds::rep>().toRawValue()};
}

auto TimeDelta::toDuration() const noexcept -> Duration {
    return Duration{_nanoseconds.converted<Seconds>()};
}

auto TimeDelta::nanoseconds(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Nanoseconds{ticks}};
}

auto TimeDelta::microseconds(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Microseconds{ticks}};
}

auto TimeDelta::milliseconds(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Milliseconds{ticks}};
}

auto TimeDelta::seconds(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Seconds{ticks}};
}

auto TimeDelta::minutes(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Minutes{ticks}};
}

auto TimeDelta::hours(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Hours{ticks}};
}

auto TimeDelta::days(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Days{ticks}};
}

auto TimeDelta::weeks(const int64_t ticks) noexcept -> TimeDelta {
    return TimeDelta{Weeks{ticks}};
}

auto TimeDelta::microsecondsOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Microseconds{ticks});
}

auto TimeDelta::millisecondsOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Milliseconds{ticks});
}

auto TimeDelta::secondsOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Seconds{ticks});
}

auto TimeDelta::minutesOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Minutes{ticks});
}

auto TimeDelta::hoursOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Hours{ticks});
}

auto TimeDelta::daysOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Days{ticks});
}

auto TimeDelta::weeksOrThrow(const int64_t ticks) -> TimeDelta {
    return createOrThrow(Weeks{ticks});
}

}
