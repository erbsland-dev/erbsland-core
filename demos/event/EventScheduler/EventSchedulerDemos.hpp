// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/EventTimer.hpp>

namespace demo {

/// State that keeps the timers of the using-timers demo alive while its event loop runs.
struct UsingTimersDemoData {
    el::EventTimerPtr quitTimer;    ///< Stops the demo after its short duration.
    el::EventTimerPtr counterTimer; ///< Prints the periodic counter value.
    int counter = 0;                ///< Next value printed by the counter timer.
};

/// Start two timers that demonstrate one-shot and fixed-delay scheduling.
void usingTimers();

/// Defer work and ignore an obsolete delayed recipient.
void queuedWork();
/// Restart, stop, and release a retained timer.
void timerLifetime();
/// Compare repeating timer modes with a slow callback.
void repeatingTimers();
/// Process the latest input after a quiet period.
void debouncing();

}
