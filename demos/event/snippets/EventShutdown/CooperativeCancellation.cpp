// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CancellableSurvey.hpp"
#include "EventShutdownDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StopSource.hpp>
#include <erbsland/String.hpp>

#include <exception>

namespace demo {

/// Cooperatively cancel one operation while its loop continues to dispatch unrelated work.
void cooperativeCancellation() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto cancellation = el::StopSource{};
    auto survey = CancellableSurvey{loop, cancellation.stopToken()};
    auto unrelatedRan = false;

    loop->invoke([&survey]() -> void { survey.advance(); });
    loop->invoke([&cancellation]() -> void {
        cancellation.requestStop();
        cancellation.requestStop(); // Repeating the request does not complete the operation twice.
        el::io::printLine("Cancellation requested; completion is still pending."_el);
    });
    loop->invoke([&unrelatedRan]() -> void {
        unrelatedRan = true;
        el::io::printLine("Unrelated callback still runs."_el);
    });
    // All borrowed state outlives the callbacks, including the survey's continuation.
    const auto dispatched = loop->runUntilIdle();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (dispatched != 4 || !survey.isCancelled() || survey.processedCount() != 1 || survey.completionCount() != 1 ||
        !unrelatedRan || loop->isQuitRequested()) {
        throw el::LogicFailure{"Expected one cancelled survey without terminating its loop."_el};
    }
}

}
