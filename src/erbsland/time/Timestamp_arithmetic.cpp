// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Timestamp.hpp"

#include "../err/OverflowError.hpp"
#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <limits>

namespace erbsland::time {

using namespace text::literals;

auto Timestamp::shifted(int64_t days, int64_t nanoseconds, bool subtract, bool &overflow) const noexcept -> Timestamp {
    overflow = false;
    if (!isValid()) {
        return {};
    }
    // Decomposition bounds both components, including for signed minimum operands.
    auto resultDays = static_cast<int64_t>(_days) + (subtract ? -days : days);
    auto resultTime = _nanoseconds + (subtract ? -nanoseconds : nanoseconds);
    if (resultTime < 0) {
        --resultDays;
        resultTime += cNanosecondsPerDay;
    } else if (resultTime >= cNanosecondsPerDay) {
        ++resultDays;
        resultTime -= cNanosecondsPerDay;
    }
    if (resultDays < 0 || resultDays > cLastDay) {
        overflow = true;
        return resultDays < 0 ? first() : last();
    }
    return Timestamp{static_cast<int32_t>(resultDays), resultTime, PrivateTag{}};
}

auto Timestamp::distanceTicks(Timestamp other, int64_t nanosecondsPerTick, bool &overflow) const noexcept -> int64_t {
    overflow = false;
    if (!isValid() || !other.isValid()) {
        return 0;
    }
    const auto dayDifference = static_cast<int64_t>(other._days) - _days;
    const auto timeDifference = other._nanoseconds - _nanoseconds;
    auto seconds = dayDifference * cSecondsPerDay + timeDifference / cNanosecondsPerSecond;
    auto fractions = timeDifference % cNanosecondsPerSecond;
    // Align the signs before truncating, so cancellation across midnight remains exact.
    if (seconds > 0 && fractions < 0) {
        --seconds;
        fractions += cNanosecondsPerSecond;
    } else if (seconds < 0 && fractions > 0) {
        ++seconds;
        fractions -= cNanosecondsPerSecond;
    }
    const auto ticksPerSecond = cNanosecondsPerSecond / nanosecondsPerTick;
    constexpr auto cMaximum = std::numeric_limits<int64_t>::max();
    constexpr auto cMinimum = std::numeric_limits<int64_t>::min();
    const auto fractionalTicks = fractions / nanosecondsPerTick;
    if (seconds > cMaximum / ticksPerSecond || seconds < cMinimum / ticksPerSecond) {
        overflow = true;
        return seconds > 0 ? cMaximum : cMinimum;
    }
    const auto wholeTicks = seconds * ticksPerSecond;
    if ((fractionalTicks > 0 && wholeTicks > cMaximum - fractionalTicks) ||
        (fractionalTicks < 0 && wholeTicks < cMinimum - fractionalTicks)) {
        overflow = true;
        return seconds >= 0 ? cMaximum : cMinimum;
    }
    return wholeTicks + fractionalTicks;
}

auto Timestamp::checkedDistanceTicks(Timestamp other, int64_t nanosecondsPerTick) const -> int64_t {
    if (!isValid() || !other.isValid()) {
        throw err::ParameterError{"A timestamp distance requires two valid instants"_el, "other"_el};
    }
    auto overflow = false;
    const auto result = distanceTicks(other, nanosecondsPerTick, overflow);
    if (overflow) {
        throw err::OverflowError{"Timestamp distance exceeds the amount range"_el};
    }
    return result;
}

auto Timestamp::secondsTo(Timestamp other) const noexcept -> Seconds {
    auto overflow = false;
    return Seconds{distanceTicks(other, 1000000000, overflow)};
}

auto Timestamp::secondsToOrThrow(Timestamp other) const -> Seconds {
    return Seconds{checkedDistanceTicks(other, 1000000000)};
}

auto Timestamp::wouldSecondsToSaturate(Timestamp other) const noexcept -> bool {
    auto overflow = false;
    [[maybe_unused]] const auto result = distanceTicks(other, 1000000000, overflow);
    return overflow;
}

auto Timestamp::millisecondsTo(Timestamp other) const noexcept -> Milliseconds {
    auto overflow = false;
    return Milliseconds{distanceTicks(other, 1000000, overflow)};
}

auto Timestamp::millisecondsToOrThrow(Timestamp other) const -> Milliseconds {
    return Milliseconds{checkedDistanceTicks(other, 1000000)};
}

auto Timestamp::wouldMillisecondsToSaturate(Timestamp other) const noexcept -> bool {
    auto overflow = false;
    [[maybe_unused]] const auto result = distanceTicks(other, 1000000, overflow);
    return overflow;
}

auto Timestamp::microsecondsTo(Timestamp other) const noexcept -> Microseconds {
    auto overflow = false;
    return Microseconds{distanceTicks(other, 1000, overflow)};
}

auto Timestamp::microsecondsToOrThrow(Timestamp other) const -> Microseconds {
    return Microseconds{checkedDistanceTicks(other, 1000)};
}

auto Timestamp::wouldMicrosecondsToSaturate(Timestamp other) const noexcept -> bool {
    auto overflow = false;
    [[maybe_unused]] const auto result = distanceTicks(other, 1000, overflow);
    return overflow;
}

auto Timestamp::nanosecondsTo(Timestamp other) const noexcept -> Nanoseconds {
    auto overflow = false;
    return Nanoseconds{distanceTicks(other, 1, overflow)};
}

auto Timestamp::nanosecondsToOrThrow(Timestamp other) const -> Nanoseconds {
    return Nanoseconds{checkedDistanceTicks(other, 1)};
}

auto Timestamp::wouldNanosecondsToSaturate(Timestamp other) const noexcept -> bool {
    auto overflow = false;
    [[maybe_unused]] const auto result = distanceTicks(other, 1, overflow);
    return overflow;
}

auto Timestamp::durationTo(Timestamp other) const noexcept -> Duration {
    return Duration{secondsTo(other)};
}

auto Timestamp::durationToOrThrow(Timestamp other) const -> Duration {
    return Duration{secondsToOrThrow(other)};
}

auto Timestamp::wouldDurationToSaturate(Timestamp other) const noexcept -> bool {
    return wouldSecondsToSaturate(other);
}

auto Timestamp::timeDeltaTo(Timestamp other) const noexcept -> TimeDelta {
    return TimeDelta{nanosecondsTo(other)};
}

auto Timestamp::timeDeltaToOrThrow(Timestamp other) const -> TimeDelta {
    return TimeDelta{nanosecondsToOrThrow(other)};
}

auto Timestamp::wouldTimeDeltaToSaturate(Timestamp other) const noexcept -> bool {
    return wouldNanosecondsToSaturate(other);
}

}
