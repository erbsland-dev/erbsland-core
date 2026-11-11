// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ChunkedSurvey.hpp"
#include "EventLoadDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <cstddef>
#include <exception>

namespace demo {

/// Let another callback run between bounded chunks without queuing the whole calculation at once.
void chunkedWork() {
    const auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto survey = ChunkedSurvey{loop};
    auto progressBetweenChunks = std::size_t{0};
    el::io::printLine("Habitat: 青龙林"_el);

    loop->invoke([&survey]() -> void { survey.advance(); });
    loop->invoke([&survey, &progressBetweenChunks]() -> void {
        progressBetweenChunks = survey.processedCount();
        el::io::printLine("Another callback sees "_el, progressBetweenChunks, " of 6 readings processed."_el);
    });
    // The stack-owned survey survives all three chunks and the observing callback.
    const auto dispatched = loop->runUntilIdle();
    if (const auto error = loop->takeError()) {
        std::rethrow_exception(error);
    }
    if (dispatched != 4 || progressBetweenChunks != 2 || survey.processedCount() != 6 || survey.total() != 17) {
        throw el::LogicFailure{"Expected another callback between the first and second chunks."_el};
    }
}

}
