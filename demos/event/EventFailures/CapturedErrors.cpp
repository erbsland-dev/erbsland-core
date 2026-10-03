// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventFailuresDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StandardStreams.hpp>

#include <exception>

namespace demo {

/// Stored callback failures can be reported after a finite batch has dispatched.
void capturedErrors() {
    auto options = el::EventLoopOptions{};
    options.setErrorPolicy(el::EventLoopErrorPolicy::Store);
    const auto loop = el::EventLoop::create(options);
    auto completed = false;
    loop->invoke([]() -> void { throw el::RuntimeError{"Route data unavailable: 地球 → 火星"_el}; });
    loop->invoke([]() -> void { throw el::RuntimeError{"Route data unavailable: 地球 → 金星"_el}; });
    loop->invoke([&completed]() -> void { completed = true; });
    const auto processed = loop->runUntilIdle();

    // Consume failures in capture order at the reporting boundary.
    auto reported = 0;
    while (const auto error = loop->takeError()) {
        try {
            std::rethrow_exception(error);
        } catch (const el::RuntimeError &failure) {
            el::io::printLine(failure.reason());
            ++reported;
        }
    }
    if (processed != 3 || !completed || reported != 2 || loop->hasError()) {
        throw el::LogicFailure{"Expected two reported errors and a completed final callback."_el};
    }
    el::io::printLine("Final callback ran; both route failures were reported."_el);
}

}
