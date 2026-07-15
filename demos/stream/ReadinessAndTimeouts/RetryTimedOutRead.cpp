// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

struct ForestReadSummary {
    std::size_t measurementCount;
    std::size_t timeoutCount;
};

[[nodiscard]] auto readForestMeasurements(el::ByteInputStream &input) -> ForestReadSummary;

void retryTimedOutRead() {
    auto input = ScriptedByteInputStream{{12U, 18U, 23U}, 1U, 1U};
    const auto summary = readForestMeasurements(input);
    el::io::printLine("Timeout: "_el, summary.timeoutCount);
    el::io::printLine("Forest measurements: "_el, summary.measurementCount);
}

/// Process data, timeout, and normal end as three separate input states.
/// Reset the stall budget whenever data arrives. A finite consecutive-timeout limit prevents a silent source from
/// keeping the workflow alive forever, while `StreamError` remains the exceptional path for a failed source.
auto readForestMeasurements(el::ByteInputStream &input) -> ForestReadSummary {
    constexpr auto cMaximumConsecutiveTimeouts = 3U;
    auto result = ForestReadSummary{};
    auto consecutiveTimeouts = 0U;

    try {
        while (true) {
            const auto readResult = input.readByte();
            if (readResult.isTimeout()) {
                ++result.timeoutCount;
                if (++consecutiveTimeouts == cMaximumConsecutiveTimeouts) {
                    throw el::RuntimeError{"The forest sensor remained inactive for too long."_el};
                }
                continue;
            }
            if (readResult.isFinished()) {
                return result;
            }

            consecutiveTimeouts = 0U;
            ++result.measurementCount;
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The forest measurements could not be read."_el, std::current_exception()};
    }
}

}
