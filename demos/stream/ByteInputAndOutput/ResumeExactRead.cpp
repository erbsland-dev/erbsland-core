// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

[[nodiscard]] auto readRecordWithTimeouts(el::ByteInputStream &input) -> el::ByteBlock;

void resumeExactRead() {
    auto input = ScriptedByteInputStream{{10U, 20U, 30U, 40U}, 2U, 1U};
    const auto record = readRecordWithTimeouts(input);
    el::io::printLine("Complete record bytes: "_el, record.length().toSizeT());
}

/// Retry a complete logical read after a timeout.
/// `readExact()` retains incomplete input internally, so always repeat the same operation with the same length.
auto readRecordWithTimeouts(el::ByteInputStream &input) -> el::ByteBlock {
    constexpr auto cMaximumAttempts = 3U;

    try {
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            auto result = input.readExact(el::ByteLength{4U});
            if (result.hasData()) {
                return result.takeData();
            }
            if (result.isFinished()) {
                throw el::RuntimeError{"The geometry record ended before it was fully read."_el};
            }

            el::io::printLine("Reading timed out; retrying the complete record."_el);
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The geometry record could not be read."_el, std::current_exception()};
    }
    throw el::RuntimeError{"Waiting for the geometry record took too long."_el};
}

}
