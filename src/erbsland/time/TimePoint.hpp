// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeDelta.hpp"

#include <chrono>
#include <compare>

namespace erbsland::time {

/// A monotonic time point for measuring elapsed time.
///
/// Wraps `std::chrono::steady_clock::time_point` and provides nanosecond-resolution arithmetic for measuring intervals.
/// @tested{TimePointTest}
class TimePoint final {
public:
    /// Create the clock epoch (zero time point).
    TimePoint() noexcept = default;
    /// Create from a `std::chrono::steady_clock::time_point`.
    /// @param value The steady clock time point.
    explicit TimePoint(std::chrono::steady_clock::time_point value) noexcept : _value{value} {}

    // defaults
    ~TimePoint() = default;
    TimePoint(const TimePoint &) noexcept = default;
    auto operator=(const TimePoint &) noexcept -> TimePoint & = default;

public:
    [[nodiscard]] auto operator<=>(const TimePoint &other) const noexcept -> std::strong_ordering {
        return _value <=> other._value;
    }
    [[nodiscard]] auto operator==(const TimePoint &other) const noexcept -> bool = default;
    [[nodiscard]] auto operator-(const TimePoint &other) const noexcept -> TimeDelta {
        return TimeDelta{_value - other._value};
    }
    [[nodiscard]] auto operator+(TimeDelta delta) const noexcept -> TimePoint {
        return TimePoint{_value + delta.toStdNanoseconds()};
    }
    auto operator+=(TimeDelta delta) noexcept -> TimePoint & {
        _value += delta.toStdNanoseconds();
        return *this;
    }
    [[nodiscard]] auto operator-(TimeDelta delta) const noexcept -> TimePoint {
        return TimePoint{_value - delta.toStdNanoseconds()};
    }
    auto operator-=(TimeDelta delta) noexcept -> TimePoint & {
        _value -= delta.toStdNanoseconds();
        return *this;
    }

public:
    /// Return the time delta to another time point.
    /// @param timePoint The target time point.
    /// @return The duration from this point to the target.
    [[nodiscard]] auto timeDeltaTo(const TimePoint &timePoint) const noexcept -> TimeDelta { return timePoint - *this; }
    /// Return the time delta to the current time.
    /// @return The duration from this point to now.
    [[nodiscard]] auto timeDeltaToNow() const noexcept -> TimeDelta { return now() - *this; }

public: // conversion
    /// Convert to a standard-library steady-clock time point.
    [[nodiscard]] auto toStdTimePoint() const noexcept -> std::chrono::steady_clock::time_point { return _value; }

public:
    /// Return the current steady clock time.
    /// @return The current time point.
    [[nodiscard]] static auto now() noexcept -> TimePoint { return TimePoint{std::chrono::steady_clock::now()}; }
    /// Create a time point in the future.
    /// @param delta The duration to add to the current time.
    /// @return A time point `delta` from now.
    [[nodiscard]] static auto inFuture(TimeDelta delta) noexcept -> TimePoint { return now() + delta; }

private:
    std::chrono::steady_clock::time_point _value;
};

}
