// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeDelta.hpp"

#include "Duration.hpp"

namespace erbsland::time {

namespace {
constexpr auto cNanosecondsPerSecond = 1000000000.0;
constexpr auto cNanosecondsPerDay = 86400.0 * cNanosecondsPerSecond;
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
    return static_cast<double>(_nanoseconds.toValue().toRawValue()) / cNanosecondsPerSecond;
}

auto TimeDelta::toDaysWithFractions() const noexcept -> double {
    return static_cast<double>(_nanoseconds.toValue().toRawValue()) / cNanosecondsPerDay;
}

auto TimeDelta::toStdNanoseconds() const noexcept -> std::chrono::nanoseconds {
    return std::chrono::nanoseconds{_nanoseconds.toValue().cast<std::chrono::nanoseconds::rep>().toRawValue()};
}

auto TimeDelta::toDuration() const noexcept -> Duration {
    return Duration{_nanoseconds.converted<Seconds>()};
}

}
