// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSchedulerDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <exception>

namespace demo {

/// An inactivity timeout is restarted by accepted input and ends the wait after input stops.
/// @notest{Compiled documentation demo.}
void inactivityTimeout() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    const auto timeout = loop->createTimer([loop]() -> void {
        el::io::printLine("Inactive observation stream closed."_el);
        loop->quit();
    });
    loop->invoke([timeout]() -> void {
        timeout->startOnce(el::Milliseconds{20});
        // Every accepted observation restarts the waiting budget.
        timeout->startOnce(el::Milliseconds{20});
    });
    loop->run();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
}

/// Restart one retained timer when input arrives, then process the most recent value after a quiet period.
/// The timer and mutable input both live until this local loop finishes.
void debouncing() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto latestObservation = el::String{};
    auto savedCount = 0;
    const auto timer = loop->createTimer([&]() -> void {
        ++savedCount;
        el::io::printLine("Saved observation: "_el, latestObservation);
        loop->quit();
    });
    loop->invoke([&]() -> void {
        for (const auto observation : {"Sazlık"_el, "Gölet"_el, "Orman"_el}) {
            latestObservation = observation;
            timer->startOnce(el::Milliseconds{20});
        }
    });
    loop->run();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (savedCount != 1) {
        throw el::LogicFailure{"Debounce must save exactly one observation."_el};
    }
    inactivityTimeout();
}

}
