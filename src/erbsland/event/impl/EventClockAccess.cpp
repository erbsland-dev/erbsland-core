// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventClockAccess.hpp"

namespace erbsland::event::impl {

std::mutex EventClockAccess::_mutex{};
EventClockAccess::NowFn EventClockAccess::_nowFn{};

auto EventClockAccess::now() -> EventTime {
    std::scoped_lock lock{_mutex};
    if (_nowFn) {
        return _nowFn();
    }
    return defaultNow();
}

auto EventClockAccess::swapNowFnForTest(NowFn nowFn) -> NowFn {
    std::scoped_lock lock{_mutex};
    auto previousNowFn = std::move(_nowFn);
    _nowFn = std::move(nowFn);
    return previousNowFn;
}

auto EventClockAccess::defaultNow() -> EventTime {
    return EventClock::now();
}

ScopedEventClockOverride::ScopedEventClockOverride(const EventTime now) :
    ScopedEventClockOverride([now]() -> EventTime { return now; }) {
}

ScopedEventClockOverride::ScopedEventClockOverride(EventClockAccess::NowFn nowFn) :
    _previousNowFn{EventClockAccess::swapNowFnForTest(std::move(nowFn))} {
}

ScopedEventClockOverride::~ScopedEventClockOverride() {
    [[maybe_unused]] auto unused = EventClockAccess::swapNowFnForTest(std::move(_previousNowFn));
}

}
