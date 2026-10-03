// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSchedulerDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTimerMode.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>

#include <exception>

namespace demo {

/// Retaining a timer permits restart and cancellation before dispatch begins.
/// An inactive timer owns its callback without scheduling it.
void timerLifetime() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto calls = 0;
    auto timer = loop->createTimer([&calls]() -> void { ++calls; });
    el::io::printLine("Initially active: "_el, timer->isActive());
    timer->startOnce(el::Seconds{10});
    timer->startOnce(el::Milliseconds{0}); // Replace the previous deadline.
    el::io::printLine("One-shot mode: "_el, timer->mode() == el::EventTimerMode::Once);
    el::io::printLine("Immediate interval: "_el, timer->interval() == el::Milliseconds{0});
    (void)loop->runUntilIdle();
    el::io::printLine("Calls after restart: "_el, calls);
    el::io::printLine("Active after dispatch: "_el, timer->isActive());

    timer->startOnce(el::Milliseconds{0});
    timer->stop();
    (void)loop->runUntilIdle();
    el::io::printLine("Calls after stop: "_el, calls);
    timer->startOnce(el::Milliseconds{0});
    timer.reset(); // Release the last handle before the occurrence is queued.
    (void)loop->runUntilIdle();
    el::io::printLine("Calls after destruction: "_el, calls);
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (calls != 1) {
        throw el::LogicFailure{"Unexpected timer callback count."_el};
    }
}

}
