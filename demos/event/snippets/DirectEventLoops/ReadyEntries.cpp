// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DirectEventLoopsDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/List.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <exception>

namespace demo {

/// Dispatch queued journal entries before resuming the procedural caller.
void readyEntries() {
    const auto loop = el::EventLoop::create();
    const auto entries = el::StringList{"抵达山口"_el, "发现石桥"_el};
    for (const auto &entry : entries) {
        loop->invoke([entry]() -> void { el::io::printLine("Journal: "_el, entry); });
    }

    el::io::printLine("Entries queued."_el);
    const auto dispatched = loop->runUntilIdle();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    el::io::printLine("Back in the host after "_el, dispatched, " callbacks."_el);
}

}
