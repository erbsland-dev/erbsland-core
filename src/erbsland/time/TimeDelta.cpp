// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeDelta.hpp"

#include "Duration.hpp"

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

auto TimeDelta::toSeconds() const noexcept -> Seconds {
    return _nanoseconds.converted<Seconds>();
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

}
