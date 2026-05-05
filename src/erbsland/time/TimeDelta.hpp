// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeAmounts.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <chrono>
#include <compare>

namespace erbsland::time {

class Duration;

/// A signed time delta with nanosecond resolution.
///
/// Represents a duration between two points in time with full nanosecond precision.
/// Arithmetic uses the saturating behavior of the underlying nanosecond amount.
/// @tested{TimeDeltaTest}
class TimeDelta final {
public:
    /// Create a zero delta.
    TimeDelta() noexcept = default;
    /// Create a delta from nanoseconds.
    /// @param nanoseconds The nanosecond value.
    explicit TimeDelta(Nanoseconds nanoseconds) noexcept : _nanoseconds{nanoseconds} {}
    /// Create a delta from any seconds-based amount.
    /// @tparam tAmount The amount type with a `SecondsUnitTag`.
    /// @param amount The amount to convert.
    template <typename tAmount>
        requires std::is_same_v<SecondsUnitTag, typename tAmount::Unit>
    explicit TimeDelta(tAmount amount) noexcept : _nanoseconds{amount.template converted<Nanoseconds>()} {}
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
    [[nodiscard]] auto operator+(TimeDelta other) const noexcept -> TimeDelta;
    auto operator+=(TimeDelta other) noexcept -> TimeDelta &;
    [[nodiscard]] auto operator-(TimeDelta other) const noexcept -> TimeDelta;
    auto operator-=(TimeDelta other) noexcept -> TimeDelta &;
    [[nodiscard]] auto operator-() const noexcept -> TimeDelta;

public: // tests
    /// Test if this delta is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _nanoseconds.isZero(); }
    /// Test if this delta is positive.
    [[nodiscard]] constexpr auto isPositive() const noexcept -> bool { return _nanoseconds.isPositive(); }
    /// Test if this delta is negative.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool { return _nanoseconds.isNegative(); }

public: // conversion
    /// Return the total nanoseconds.
    [[nodiscard]] constexpr auto toNanoseconds() const noexcept -> Nanoseconds { return _nanoseconds; }
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

public:
    /// Return a zero delta.
    [[nodiscard]] static auto zero() noexcept -> TimeDelta { return {}; }

private:
    Nanoseconds _nanoseconds; ///< Total nanoseconds.
};

}
