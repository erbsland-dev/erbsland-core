// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

/// Write one orchestra score into a stream that remains owned by the caller.
/// The borrowed stream pointer keeps the stream alive while this operation runs, but this function does not close or
/// abort it. The owner decides when all writing is finished and how to handle its close result.
void writeOrchestraScore(const el::ByteOutputStreamPtr &output) {
    output->writeUInt16(72U);
}

/// Finish owned output explicitly so every accepted byte is delivered before the stream is released.
/// A close call is bounded by the stream timeout. If this workflow cannot wait beyond the first deadline, abort the
/// still-closing stream and report incomplete delivery instead of silently relying on destruction.
void closeGracefully() {
    const el::ByteOutputStreamPtr output = std::make_shared<ScriptedByteOutputStream>();
    writeOrchestraScore(output);

    try {
        if (output->close().isTimeout()) {
            output->abort();
            throw el::RuntimeError{"The orchestra score could not be saved within the close timeout."_el};
        }
        el::io::printLine("Orchestra score closed safely."_el);
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The orchestra score could not be closed."_el, std::current_exception()};
    }

    el::io::printLine("Final state Closed: "_el, output->state() == el::StreamState::Closed);
}

}
