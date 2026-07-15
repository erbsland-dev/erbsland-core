// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

namespace {
struct UsingTimersDemoData {
    el::EventTimerPtr quitTimer;
    el::EventTimerPtr counterTimer;
    int counter = 0;
} demoData;
}

void printCounter();

/// Writing event-based applications using scheduled timers is straight forward.
/// Compared with common script languages, event timers are bound to the lifetime of the returned event timer.
/// You need to store this timer instance to keep the timer alive.
/// This method is called once, when the demo is started, before it enters the main loop.
void usingTimers() {
    const auto events = el::application().events();
    demoData.quitTimer = events->createTimer([]() -> void { el::application().quit(); });
    demoData.quitTimer->startOnce(el::Seconds{3});
    demoData.counterTimer = events->createTimer(printCounter);
    demoData.counterTimer->startFixedDelay(el::Milliseconds{500});
}

void printCounter() {
    el::stdOut()->printLine("Counter: ", demoData.counter++);
}

}
