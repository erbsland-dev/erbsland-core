// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadingBatch.hpp"

#include <erbsland/BindEvent.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StandardStreams.hpp>

#include <array>
#include <utility>

namespace demo {

ReadingBatch::ReadingBatch(el::EventLoopPtr loop) : _loop{std::move(loop)} {
}

void ReadingBatch::start() {
    launchAvailable();
}

void ReadingBatch::launchAvailable() {
    const auto receiveReading =
        el::bindEvent(_loop, [this](std::size_t index, int value) -> void { receive(index, value); });
    constexpr auto readings = std::array{4, 7, 2};
    while (_next < readings.size() && _inFlight < 2) {
        const auto index = _next++;
        const auto value = readings[index];
        ++_inFlight;
        // Stand-in for independent asynchronous sources; this callback still runs on the same loop.
        receiveReading(index, value);
    }
}

void ReadingBatch::receive(const std::size_t index, const int value) {
    --_inFlight;
    ++_completed;
    _sum += value;
    el::io::printLine("Reading "_el, index, " received: "_el, value);
    if (_completed == 3) {
        if (_sum != 13 || _inFlight != 0) {
            throw el::LogicFailure{"Unexpected batch result."_el};
        }
        el::io::printLine("Batch complete; total observations: "_el, _sum);
        _loop->quit();
        return;
    }
    launchAvailable();
}

// End of batch implementation.

}
