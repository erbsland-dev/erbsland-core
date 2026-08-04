// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSchedulerDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

/// Writing event-based applications using scheduled timers is straight forward.
/// Compared with common script languages, event timers are bound to the lifetime of the returned event timer.
/// You need to store this timer instance to keep the timer alive.
/// This method is called once, when the demo is started, before it enters the main loop.
void usingTimers() {
    static auto demoData = UsingTimersDemoData{};
    const auto events = el::application().events();
    demoData.quitTimer = events->createTimer([]() -> void { el::application().quit(); });
    demoData.quitTimer->startOnce(el::Seconds{3});
    demoData.counterTimer =
        events->createTimer([]() -> void { el::stdOut()->printLine("Counter: ", demoData.counter++); });
    demoData.counterTimer->startFixedDelay(el::Milliseconds{500});
}

}
