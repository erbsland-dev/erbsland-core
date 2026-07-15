// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void waitForNextObservation(el::ByteInputStream &input);

void waitForInput() {
    auto input = ScriptedByteInputStream{{21U}, 1U, 0U, false};
    waitForNextObservation(input);
}

/// Wait for readiness without surrendering control indefinitely.
/// Each `waitForReady()` call uses the stream timeout. A separate attempt limit gives this workflow an overall bound
/// and leaves a timeout available for cancellation checks, progress updates, or other scheduled work.
void waitForNextObservation(el::ByteInputStream &input) {
    constexpr auto cMaximumWaits = 3U;

    try {
        for (auto attempt = 1U; attempt <= cMaximumWaits; ++attempt) {
            if (input.waitForReady().isReady()) {
                el::io::printLine("Sensor ready after "_el, attempt, " waits."_el);
                return;
            }
            el::io::printLine("Wait timed out; checking whether the work is still needed."_el);
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The forest sensor could not be awaited."_el, std::current_exception()};
    }
    throw el::RuntimeError{"The sensor did not become ready within the observation limit."_el};
}

}
