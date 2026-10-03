// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EventFailuresDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorAction.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StandardStreams.hpp>

#include <exception>

namespace demo {

/// A custom handler can stop dispatch immediately after a callback failure.
void errorHandler() {
    const auto loop = el::EventLoop::create();
    auto handled = false;
    auto nextCallbackRan = false;
    loop->setErrorHandler([&handled](std::exception_ptr error) -> el::EventLoopErrorAction {
        handled = error != nullptr;
        return el::EventLoopErrorAction::Stop;
    });
    loop->invoke([]() -> void { throw el::RuntimeError{"Route calculation failed: 地球 → 火星"_el}; });
    loop->invoke([&nextCallbackRan]() -> void { nextCallbackRan = true; });
    const auto processed = loop->runUntilIdle();

    const auto error = loop->takeError();
    if (processed != 1 || !handled || nextCallbackRan || !error) {
        throw el::LogicFailure{"Expected a captured error and stopped dispatch."_el};
    }
    try {
        std::rethrow_exception(error);
    } catch (const el::RuntimeError &failure) {
        el::io::printLine(failure.reason());
    }
    loop->setErrorHandler({}); // Restore the configured default policy.
    el::io::printLine("Dispatch stopped before the next callback."_el);
}

}
