// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void readAvailableObservation(el::ByteInputStream &input);

void checkReadiness() {
    auto input = ScriptedByteInputStream{{1U, 2U, 3U}, 3U, 0U, false};
    readAvailableObservation(input);
}

/// Use `isReady()` when the current thread must not wait.
/// Readiness is only a snapshot: it neither consumes data nor reserves it. If the stream is ready, still inspect the
/// following read result because another consumer or a state change can invalidate the snapshot.
void readAvailableObservation(el::ByteInputStream &input) {
    if (!input.isReady()) {
        el::io::printLine("The sensor is not ready yet; continuing with other work."_el);
        return;
    }

    try {
        const auto result = input.readByte();
        if (result.isTimeout()) {
            el::io::printLine("Readiness changed before the read."_el);
        } else if (result.isFinished()) {
            el::io::printLine("The observation series is complete."_el);
        } else {
            el::io::printLine("Available observation: "_el, result.data().toUInt8());
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The forest sensor could not be read."_el, std::current_exception()};
    }
}

}
