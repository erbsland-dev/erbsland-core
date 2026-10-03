// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventLoadDemos.hpp"
#include "ReadingBatch.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>

#include <exception>

namespace demo {

/// Independent waits can share one completion point without creating a thread for every request.
/// This bounded batch is retained on the stack until its local loop finishes dispatching.
void collectReadings() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto batch = ReadingBatch{loop};
    loop->invoke([&batch]() -> void { batch.start(); });
    loop->run();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
}

}
