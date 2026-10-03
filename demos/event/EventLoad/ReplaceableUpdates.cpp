// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventLoadDemos.hpp"

#include <erbsland/BindEvent.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventQueueOverflowAction.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <exception>
#include <vector>

namespace demo {

/// Replaceable progress samples may be discarded when their display loop is full.
void replaceableUpdates() {
    auto options = el::EventLoopOptions{};
    options.setMaximumQueuedEvents(2);
    options.setQueueOverflowAction(el::EventQueueOverflowAction::Ignore);
    const auto loop = el::EventLoop::create(options);
    auto displayed = std::vector<int>{};
    const auto route = el::String{"地球 → 火星"_el};

    const auto displayProgress = el::bindEvent(loop, [&displayed, route](int progress) -> void {
        displayed.push_back(progress);
        el::io::printLine(route, ": "_el, progress, "%"_el);
    });
    // Fill the queue before dispatch, making this burst deterministic.
    for (const auto progress : {10, 20, 30, 40}) {
        displayProgress(progress);
    }
    const auto processed = loop->runUntilIdle();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (processed != 2 || displayed != std::vector<int>{10, 20}) {
        throw el::LogicFailure{"Expected only the two previously accepted updates."_el};
    }
    el::io::printLine("Two samples displayed; two incoming samples discarded."_el);
}

}
