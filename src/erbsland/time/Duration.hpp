// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Duration_fwd.hpp"
#include "DurationPart.hpp"
#include "TimeAmounts.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <chrono>
#include <compare>

namespace erbsland::time {

class TimeDelta;

/// A signed duration with a resolution of seconds.
///
/// Represents a span of time measured in seconds, with helper methods to
/// split into days, hours, minutes, and seconds components.
/// Arithmetic uses the saturating behavior of the underlying second amount.
/// @tested{DurationTest}
class Duration final {
public:
    /// Split duration parts.
    struct Parts {
        Seconds seconds; ///< The seconds component.
        Minutes minutes; ///< The minutes component.
        Hours hours;     ///< The hours component.
        Days days;       ///< The days component.
        Weeks weeks;     ///< The weeks component.
    };

    /// Split duration into whole days and a signed sub-day nanosecond remainder.
    /// @tested{TimeCoreTest}
    struct DaysAndNanoseconds {
        Days days;               ///< Whole days, truncated toward zero.
        Nanoseconds nanoseconds; ///< Signed sub-day nanosecond remainder.
    };

public:
    /// Create a zero duration.
    Duration() noexcept = default;
    /// Create a duration from seconds.
    /// @param seconds The second value.
    explicit Duration(const Seconds seconds) noexcept : _seconds{seconds} {}
    /// Create a duration from any seconds-based amount.
    /// @tparam tAmount The amount type with a `SecondsUnitTag`.
    /// @param amount The amount to convert.
    template <typename tAmount>
        requires std::is_same_v<SecondsUnitTag, typename tAmount::Unit>
    explicit Duration(tAmount amount) noexcept : _seconds{amount.template converted<Seconds>()} {}
    /// Create a duration from a `std::chrono::duration`, truncating to seconds.
    /// @tparam tRep The representation type.
    /// @tparam tPeriod The period type.
    /// @param duration The chrono duration to convert.
    template <typename tRep, typename tPeriod>
    explicit Duration(std::chrono::duration<tRep, tPeriod> duration) noexcept :
        _seconds{std::chrono::duration_cast<std::chrono::seconds>(duration).count()} {}
    /// Create a duration from split parts.
    /// @param parts The parts to combine.
    explicit Duration(Parts parts) noexcept;

    // defaults
    ~Duration() = default;
    Duration(const Duration &) noexcept = default;
    auto operator=(const Duration &) noexcept -> Duration & = default;
    Duration(Duration &&) noexcept = default;
    auto operator=(Duration &&) noexcept -> Duration & = default;

public: // operators
    [[nodiscard]] auto operator<=>(const Duration &other) const noexcept -> std::strong_ordering = default;
    [[nodiscard]] auto operator+(Duration other) const noexcept -> Duration;
    auto operator+=(Duration other) noexcept -> Duration &;
    [[nodiscard]] auto operator-(Duration other) const noexcept -> Duration;
    auto operator-=(Duration other) noexcept -> Duration &;
    [[nodiscard]] auto operator-() const noexcept -> Duration;

public: // tests
    /// Test if this duration is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _seconds.isZero(); }
    /// Test if the duration is positive.
    [[nodiscard]] constexpr auto isPositive() const noexcept -> bool { return _seconds.isPositive(); }
    /// Test if the duration is negative.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool { return _seconds.isNegative(); }

public: // accessors
    /// Return the seconds component (0-59).
    /// @return The seconds component within the minute.
    [[nodiscard]] auto seconds() const noexcept -> Seconds;
    /// Return the minutes component (0-59).
    /// @return The minutes component within the hour.
    [[nodiscard]] auto minutes() const noexcept -> Minutes;
    /// Return the hours component (0-23).
    /// @return The hours component within the day.
    [[nodiscard]] auto hours() const noexcept -> Hours;
    /// Return the days component.
    /// @return The total number of days.
    [[nodiscard]] auto days() const noexcept -> Days;
    /// Split this duration into day/hour/minute/second parts.
    ///
    /// The `largestPart` parameter controls which unit is used as the top-level
    /// component. For example, `DurationPart::Weeks` includes weeks in the result. Negative durations keep negative
    /// signs in their extracted components.
    /// @param largestPart The largest unit to include.
    /// @return Named weeks, days, hours, minutes, and seconds parts.
    [[nodiscard]] auto parts(DurationPart largestPart = DurationPart::Days) const noexcept -> Parts;

public: // conversion
    /// Return the total seconds.
    /// @return The total second value.
    [[nodiscard]] constexpr auto toSeconds() const noexcept -> Seconds { return _seconds; }
    /// Convert to `std::chrono::seconds`.
    /// @return The equivalent chrono duration.
    [[nodiscard]] auto toStdSeconds() const noexcept -> std::chrono::seconds;
    /// Split into whole days and a signed sub-day nanosecond remainder.
    /// @return Whole days and remaining nanoseconds.
    [[nodiscard]] auto toDaysAndNanoseconds() const noexcept -> DaysAndNanoseconds;
    /// Convert to days with fractions.
    /// @return The total signed duration in days.
    [[nodiscard]] auto toDaysWithFractions() const noexcept -> double;
    /// Test if conversion to `TimeDelta` would saturate.
    /// @return `true` if `toTimeDelta()` would return a saturated nanosecond value.
    [[nodiscard]] auto wouldConvertToTimeDeltaSaturate() const noexcept -> bool;
    /// Convert to a time delta with nanosecond precision, saturating if outside nanosecond bounds.
    /// @return The saturated time delta.
    [[nodiscard]] auto toTimeDelta() const noexcept -> TimeDelta;
    /// Convert to a time delta with nanosecond precision.
    /// @return The time delta.
    /// @throws err::OverflowError if the conversion would saturate.
    [[nodiscard]] auto toTimeDeltaOrThrow() const -> TimeDelta;

public:
    /// Return a zero duration.
    /// @return A zero-valued `Duration`.
    [[nodiscard]] static auto zero() noexcept -> Duration { return {}; }

private:
    Seconds _seconds; ///< Total seconds.
};

}
