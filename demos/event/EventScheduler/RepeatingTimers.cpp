// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSchedulerDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/EventTimerMode.hpp>
#include <erbsland/StandardStreams.hpp>

#include <chrono>
#include <exception>
#include <thread>

namespace demo {

/// Compare fixed-delay and fixed-rate timers with deliberately slow callbacks.
/// This sleep illustrates a blocked loop; production callbacks should stay short.
void repeatingTimers() {
    for (const auto mode : {el::EventTimerMode::FixedDelay, el::EventTimerMode::FixedRate}) {
        const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
        auto timer = el::EventTimerPtr{};
        auto count = 0;
        const auto start = std::chrono::steady_clock::now();
        timer = loop->createTimer([&]() -> void {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            el::io::printLine("Tick "_el, ++count, " at about "_el, elapsed, " ms"_el);
            std::this_thread::sleep_for(std::chrono::milliseconds{35});
            if (count == 3) {
                timer->stop(); // A running callback may stop its next occurrence.
                loop->quit();
            }
        });
        if (mode == el::EventTimerMode::FixedDelay) {
            el::io::printLine("Fixed delay:"_el);
            timer->startFixedDelay(el::Milliseconds{20});
        } else {
            el::io::printLine("Fixed rate:"_el);
            timer->startFixedRate(el::Milliseconds{20});
        }
        loop->run();
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }
}

}
