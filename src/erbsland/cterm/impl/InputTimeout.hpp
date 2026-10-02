// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../time/TimePoint.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace erbsland::cterm::impl {

/// A finite input deadline or an explicit unbounded wait.
/// @tested{InputTimeoutTest}
class InputTimeout final {
public:
    /// Create an unbounded wait, or a finite wait starting at the given monotonic time.
    /// @param timeout A finite timeout; negative values become a nonblocking poll. No value means unbounded.
    /// @param now The start time, injectable for deterministic tests.
    explicit InputTimeout(
        std::optional<time::Milliseconds> timeout = {}, time::TimePoint now = time::TimePoint::now()) noexcept {
        if (timeout.has_value()) {
            const auto duration = time::TimeDelta{std::max(*timeout, time::Milliseconds{})};
            const auto capacity = time::TimeDelta{time::Nanoseconds::maximum()} - time::TimePoint{}.timeDeltaTo(now);
            _deadline = now + std::min(duration, capacity);
        }
    }
    /// Get the remaining finite timeout, rounding positive fractions up to a millisecond.
    /// @param now The current monotonic time.
    /// @return No value for an unbounded wait; zero when the deadline has elapsed.
    [[nodiscard]] auto remaining(time::TimePoint now = time::TimePoint::now()) const noexcept
        -> std::optional<time::Milliseconds> {
        if (!_deadline.has_value()) {
            return {};
        }
        return roundedMilliseconds(now.timeDeltaTo(*_deadline));
    }
    /// Test whether a finite deadline has elapsed.
    [[nodiscard]] auto expired(time::TimePoint now = time::TimePoint::now()) const noexcept -> bool {
        return _deadline.has_value() && now >= *_deadline;
    }
    /// Round a positive delta up to milliseconds, clamping negative deltas to zero.
    /// @param delta The remaining duration.
    /// @return The rounded nonnegative timeout.
    [[nodiscard]] static auto roundedMilliseconds(time::TimeDelta delta) noexcept -> time::Milliseconds {
        const auto ticks = delta.toNanoseconds().toRawValue();
        return time::Milliseconds{ticks <= 0 ? 0 : ticks / 1'000'000 + (ticks % 1'000'000 != 0)};
    }
    /// Convert a finite timeout without producing the Windows INFINITE sentinel.
    /// @param timeout The finite wait.
    /// @return The bounded native millisecond value.
    [[nodiscard]] static auto windowsMilliseconds(time::Milliseconds timeout) noexcept -> uint32_t {
        return static_cast<uint32_t>(
            std::clamp<int64_t>(timeout.toRawValue(), 0, std::numeric_limits<uint32_t>::max() - 1));
    }
    /// Split a finite timeout into POSIX seconds and microseconds without narrowing intermediate values.
    /// @param timeout The finite wait.
    /// @return Nonnegative seconds and a microsecond remainder below one million.
    [[nodiscard]] static auto posixParts(time::Milliseconds timeout) noexcept -> std::pair<int64_t, int64_t> {
        const auto ticks = std::max<int64_t>(timeout.toRawValue(), 0);
        return {ticks / 1000, (ticks % 1000) * 1000};
    }

private:
    std::optional<time::TimePoint> _deadline; ///< Absent only for an unbounded wait.
};

}
