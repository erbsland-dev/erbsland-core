// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Event.hpp"

#include <functional>
#include <mutex>

namespace erbsland::event::impl {

/// Internal access wrapper around the UI event clock.
class EventClockAccess {
public:
    using NowFn = std::function<EventTime()>;

public:
    /// Read the current event time.
    /// @return The current time.
    [[nodiscard]] static auto now() -> EventTime;
    /// Swap the clock function used by `now()`.
    /// @param nowFn The replacement clock function. An empty function restores the default clock.
    /// @return The previously active clock function. An empty function indicates the default clock.
    [[nodiscard]] static auto swapNowFnForTest(NowFn nowFn) -> NowFn;

private:
    /// The default clock function.
    /// @return The current steady-clock time.
    [[nodiscard]] static auto defaultNow() -> EventTime;

private:
    static std::mutex _mutex; ///< Protects the clock override function.
    static NowFn _nowFn;      ///< Optional override clock for tests.
};

/// Scoped override for the internal UI clock.
class ScopedEventClockOverride final {
public:
    /// Override the internal clock with a fixed time point.
    /// @param now The time point returned by `EventClockAccess::now()`.
    explicit ScopedEventClockOverride(EventTime now);
    /// Override the internal clock with a custom function.
    /// @param nowFn The replacement clock function.
    explicit ScopedEventClockOverride(EventClockAccess::NowFn nowFn);

    // defaults
    ~ScopedEventClockOverride();
    ScopedEventClockOverride(const ScopedEventClockOverride &) = delete;
    ScopedEventClockOverride(ScopedEventClockOverride &&) = delete;
    auto operator=(const ScopedEventClockOverride &) -> ScopedEventClockOverride & = delete;
    auto operator=(ScopedEventClockOverride &&) -> ScopedEventClockOverride & = delete;

private:
    EventClockAccess::NowFn _previousNowFn; ///< The previously active override.
};

}
