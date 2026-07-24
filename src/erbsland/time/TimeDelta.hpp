// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Duration_fwd.hpp"
#include "TimeAmounts.hpp"
#include "TimeDeltaFormat.hpp"

#include "../text/FormatAs.hpp"
#include "../text/String.hpp"
#include "../text/StringConverter.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <chrono>
#include <compare>

namespace erbsland::time {

/// A signed time delta with nanosecond resolution.
///
/// Represents a duration between two points in time with full nanosecond precision.
/// Arithmetic uses the saturating behavior of the underlying nanosecond amount.
/// @tested{TimeCoreTest}
class TimeDelta final {
public:
    /// The native integer type this delta is based on.
    using IntegerValue = Nanoseconds::Value;

public:
    /// Create a zero delta.
    TimeDelta() noexcept = default;
    /// Create a delta from nanoseconds.
    /// @param nanoseconds The nanosecond value.
    explicit TimeDelta(const Nanoseconds nanoseconds) noexcept : _nanoseconds{nanoseconds} {}
    /// Create a delta from any seconds-based amount.
    /// Saturates if the value exceeds the maximum representable value.
    /// @tparam tAmount The amount type with a `SecondsUnitTag`.
    /// @param amount The amount to convert.
    template <typename tAmount>
        requires std::is_same_v<SecondsUnitTag, typename tAmount::Unit>
    constexpr TimeDelta(tAmount amount) noexcept : // NOLINT(*-explicit-constructor)
        _nanoseconds{amount.template converted<Nanoseconds>()} {}
    /// Create a delta from a `std::chrono::duration`.
    /// @tparam tRep The representation type.
    /// @tparam tPeriod The period type.
    /// @param duration The chrono duration to convert.
    template <typename tRep, typename tPeriod>
    explicit TimeDelta(std::chrono::duration<tRep, tPeriod> duration) noexcept :
        _nanoseconds{std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()} {}

    // defaults
    ~TimeDelta() = default;
    TimeDelta(const TimeDelta &) noexcept = default;
    auto operator=(const TimeDelta &) noexcept -> TimeDelta & = default;
    TimeDelta(TimeDelta &&) noexcept = default;
    auto operator=(TimeDelta &&) noexcept -> TimeDelta & = default;

public: // operators
    [[nodiscard]] auto operator<=>(const TimeDelta &other) const noexcept -> std::strong_ordering = default;
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const TimeDelta &other, other._nanoseconds);

    /// Add two time deltas.
    [[nodiscard]] auto operator+(TimeDelta other) const noexcept -> TimeDelta;
    /// Add a time delta.
    auto operator+=(TimeDelta other) noexcept -> TimeDelta &;
    /// Subtract two time deltas.
    [[nodiscard]] auto operator-(TimeDelta other) const noexcept -> TimeDelta;
    /// Subtract a time delta.
    auto operator-=(TimeDelta other) noexcept -> TimeDelta &;
    /// Negate this time delta.
    [[nodiscard]] auto operator-() const noexcept -> TimeDelta;
    /// Divide this time delta.
    /// By dividing the time-delta with regular integer, the result is a time-delta.
    [[nodiscard]] auto operator/(IntegerValue divisor) const noexcept -> TimeDelta;
    /// Divide this time delta.
    auto operator/=(IntegerValue divisor) noexcept -> TimeDelta &;
    /// By dividing the time-delta with another time-delta, the result is a regular integer.
    [[nodiscard]] auto operator/(TimeDelta divisor) const noexcept -> IntegerValue;
    /// Multiply this time delta.
    [[nodiscard]] auto operator*(IntegerValue factor) const noexcept -> TimeDelta;
    /// Multiply this time delta.
    auto operator*=(IntegerValue factor) noexcept -> TimeDelta &;

public: // tests
    /// Test if this delta is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _nanoseconds.isZero(); }
    /// Test if this delta is positive.
    [[nodiscard]] constexpr auto isPositive() const noexcept -> bool { return _nanoseconds.isPositive(); }
    /// Test if this delta is negative.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool { return _nanoseconds.isNegative(); }

public: // tools
    /// Get the absolute value of this delta.
    [[nodiscard]] auto toAbsolute() const noexcept -> TimeDelta { return _nanoseconds.toAbsolute(); }
    /// Return a time delta that is at minimum the given value.
    template <typename tAmount>
        requires std::is_same_v<SecondsUnitTag, typename tAmount::Unit>
    [[nodiscard]] auto minimum(tAmount amount) const noexcept -> TimeDelta {
        return std::max(*this, TimeDelta{amount});
    }

public: // conversion
    /// Convert this delta to a human-readable string.
    [[nodiscard]] auto toString(const TimeDeltaFormat &format = {}) const -> text::String;
    /// Return the total nanoseconds.
    [[nodiscard]] constexpr auto toNanoseconds() const noexcept -> Nanoseconds { return _nanoseconds; }
    /// Return the total milliseconds, truncating sub-milliseconds nanoseconds toward zero.
    [[nodiscard]] auto toMilliseconds() const noexcept -> Milliseconds;
    /// Return the total seconds, truncating sub-second nanoseconds toward zero.
    [[nodiscard]] auto toSeconds() const noexcept -> Seconds;
    /// Return the total seconds as a floating-point value.
    [[nodiscard]] auto toSecondsWithFractions() const noexcept -> double;
    /// Return the total days as a floating-point value.
    [[nodiscard]] auto toDaysWithFractions() const noexcept -> double;
    /// Convert to `std::chrono::nanoseconds`.
    [[nodiscard]] auto toStdNanoseconds() const noexcept -> std::chrono::nanoseconds;
    /// Convert to a duration, truncating sub-second nanoseconds toward zero.
    [[nodiscard]] auto toDuration() const noexcept -> Duration;

public: // factory methods
    /// Return a zero delta.
    [[nodiscard]] static auto zero() noexcept -> TimeDelta { return {}; }
    /// Return a time delta in nanoseconds.
    [[nodiscard]] static auto nanoseconds(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in microseconds.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto microseconds(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in milliseconds.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto milliseconds(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in seconds.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto seconds(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in minutes.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto minutes(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in hours.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto hours(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in days.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto days(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in weeks.
    /// If the value exceeds the maximum representable value, it saturates to the maximum.
    [[nodiscard]] static auto weeks(int64_t ticks) noexcept -> TimeDelta;
    /// Return a time delta in microseconds.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto microsecondsOrThrow(int64_t ticks) -> TimeDelta;
    /// Return a time delta in milliseconds.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto millisecondsOrThrow(int64_t ticks) -> TimeDelta;
    /// Return a time delta in seconds.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto secondsOrThrow(int64_t ticks) -> TimeDelta;
    /// Return a time delta in minutes.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto minutesOrThrow(int64_t ticks) -> TimeDelta;
    /// Return a time delta in hours.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto hoursOrThrow(int64_t ticks) -> TimeDelta;
    /// Return a time delta in days.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto daysOrThrow(int64_t ticks) -> TimeDelta;
    /// Return a time delta in weeks.
    /// @throws err::OverflowError If the value exceeds the maximum representable value.
    [[nodiscard]] static auto weeksOrThrow(int64_t ticks) -> TimeDelta;

private:
    template <typename tTimeUnit>
    [[nodiscard]] static auto createOrThrow(tTimeUnit value) -> TimeDelta;

    Nanoseconds _nanoseconds; ///< Total nanoseconds.
};

}

template <>
struct erbsland::text::FormatAsText<erbsland::time::TimeDelta> : FormatAs<time::TimeDelta, String> {
    [[nodiscard]] auto format(const time::TimeDelta &value) const -> String { return value.toString(); }
};
