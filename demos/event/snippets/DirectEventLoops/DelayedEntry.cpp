// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DirectEventLoopsDemos.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CurrentLoop.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/TimePoint.hpp>

#include <exception>

namespace demo {

/// Give the loop short turns until a delayed journal entry arrives or the host's deadline expires.
void delayedEntry() {
    auto delivered = false;
    const auto loop = el::EventLoop::create();
    loop->invoke([&delivered]() -> void {
        // Dispatch has established the current loop for this callback.
        el::currentLoop()->invokeAfter(el::Milliseconds{30}, [&delivered]() -> void {
            el::io::printLine("Journal: "_el, "月下渡河"_el);
            delivered = true;
        });
    });

    const auto deadline = el::TimePoint::inFuture(el::Seconds{2});
    while (!delivered) {
        if (el::TimePoint::now() >= deadline) {
            throw el::ApplicationError{"Timed out waiting for the journal entry."_el};
        }
        (void)loop->runOnce(el::Milliseconds{5});
        if (const auto error = loop->takeError()) {
            std::rethrow_exception(error);
        }
    }
    el::io::printLine("The host has received the journal entry."_el);
}

}
