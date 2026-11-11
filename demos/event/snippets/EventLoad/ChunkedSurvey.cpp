// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ChunkedSurvey.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <array>
#include <utility>

namespace demo {

ChunkedSurvey::ChunkedSurvey(el::EventLoopPtr loop) : _loop{std::move(loop)} {
}

void ChunkedSurvey::advance() {
    constexpr auto readings = std::array{2, 4, 1, 3, 5, 2};
    constexpr auto chunkSize = std::size_t{2};
    auto processed = std::size_t{0};
    while (_next < readings.size() && processed < chunkSize) {
        _total += readings[_next];
        ++_next;
        ++processed;
    }
    el::io::printLine("Readings processed: "_el, _next);
    if (_next < readings.size()) {
        // Schedule only the next chunk, leaving other callbacks an opportunity to run.
        _loop->invoke([this]() -> void { advance(); });
    } else {
        el::io::printLine("Survey complete; observations: "_el, _total);
    }
}

}
