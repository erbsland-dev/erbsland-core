// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSchedulerDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("Debouncing"_el, debouncing);
    app.registerDemo("QueuedWork"_el, queuedWork);
    app.registerDemo("RepeatingTimers"_el, repeatingTimers);
    app.registerDemo("TimerLifetime"_el, timerLifetime);
    app.registerDemo("UsingTimers"_el, usingTimers, DemoApplication::Mode::EventLoop);
    return app.run();
}
