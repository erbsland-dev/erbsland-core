// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

[[nodiscard]] auto writeObservationRecord(el::ByteOutputStream &output, const el::ByteBlock &record) -> std::size_t;

void retryAtomicWrite() {
    auto output = ScriptedByteOutputStream{1U};
    const auto observation = el::ByteBlock({0x17U, 0x04U, 0x2aU});
    const auto attempts = writeObservationRecord(output, observation);
    el::io::printLine("Attempts: "_el, attempts);
    el::io::printLine("Record accepted once: "_el, output.bytes().size() == observation.length().toSizeT());
}

/// Retry one complete output request without changing it.
/// A timed-out write accepted none of the record, so repeating the same call cannot duplicate a partial record.
/// Limit retries, handle stream failures separately, and flush accepted output when native delivery matters.
auto writeObservationRecord(el::ByteOutputStream &output, const el::ByteBlock &record) -> std::size_t {
    constexpr auto cMaximumAttempts = 3U;

    try {
        for (auto attempt = 1U; attempt <= cMaximumAttempts; ++attempt) {
            if (output.write(record).isTimeout()) {
                continue;
            }
            for (auto flushAttempt = 0U; flushAttempt < cMaximumAttempts; ++flushAttempt) {
                if (output.flush().isSuccess()) {
                    return attempt;
                }
            }
            throw el::RuntimeError{"The record was accepted but was not sent within the expected time."_el};
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The forest observation could not be recorded."_el, std::current_exception()};
    }
    throw el::RuntimeError{"The recorder did not accept the record within the expected time."_el};
}

}
