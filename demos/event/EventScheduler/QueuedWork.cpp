// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventSchedulerDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <exception>
#include <memory>

namespace demo {

/// Invocations defer work on one loop, even when posted from that loop's own callback.
/// Delayed invocations retain their callback without a cancellation handle.
void queuedWork() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto recipient = std::make_shared<el::String>("Sulak alan"_el);
    const auto weakRecipient = std::weak_ptr<el::String>{recipient};
    loop->invoke([loop]() -> void {
        el::io::printLine("Preparing observation."_el);
        loop->invoke([]() -> void { el::io::printLine("Queued observation."_el); });
        el::io::printLine("Preparation returned before observation."_el);
    });
    loop->invokeAfter(el::Milliseconds{-1}, [weakRecipient]() -> void {
        if (const auto owner = weakRecipient.lock()) {
            el::io::printLine("Observation: "_el, *owner);
        } else {
            el::io::printLine("Obsolete observation ignored."_el);
        }
    });
    recipient.reset();
    (void)loop->runUntilIdle();
    loop->invokeAfter(el::Milliseconds{5}, [&loop, caption = "Sulak alan"_el]() -> void {
        el::io::printLine("Delayed reminder: "_el, caption);
        loop->quit();
    });
    loop->run();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
}

}
