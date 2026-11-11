// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventLoadDemos.hpp"
#include "LatestProgress.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/List.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <exception>
#include <thread>

namespace demo {

/// Display the newest value of a burst, then accept another notification after consumption.
void coalescedUpdates() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto displayed = el::List<int>{};
    auto updates = LatestProgress{loop, [&displayed](const int progress) -> void {
                                      displayed.append(progress);
                                      el::io::printLine("青龙林 survey: "_el, progress, "%"_el);
                                  }};

    // Join this finite producer before dispatch to make the captured burst deterministic.
    auto producer = std::jthread{[&updates]() -> void {
        for (const auto progress : {10, 20, 30, 40}) {
            updates.publish(progress);
        }
    }};
    producer.join();
    if (loop->queuedEventCount() != 1) {
        throw el::LogicFailure{"Expected one queued notification for the whole burst."_el};
    }
    const auto firstDispatch = loop->runUntilIdle();

    // Consumption clears the pending flag, so a later burst schedules a fresh notification.
    updates.publish(60);
    updates.publish(80);
    const auto secondDispatch = loop->runUntilIdle();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (firstDispatch != 1 || secondDispatch != 1 || displayed != el::List<int>{40, 80}) {
        throw el::LogicFailure{"Expected the latest value of each burst."_el};
    }
}

}
