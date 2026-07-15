// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Write an orchestra cue to a stream that remains owned by the caller.
/// A borrowed stream pointer keeps the shared stream alive for this operation, but the caller retains its lifecycle.
void writeOrchestraCue(const el::ByteOutputStreamPtr &output) {
    output->writeUInt8(64U);
}

/// Stop using a stream after a native failure and report its diagnostic at the application boundary.
/// `StreamError` is different from a routine timeout: the stream enters `Failed`, and later operations surface the
/// stored failure. Discard this instance and create a new stream only when the application has a recovery strategy.
void observeFailedState() {
    const auto scriptedOutput = std::make_shared<ScriptedByteOutputStream>();
    const el::ByteOutputStreamPtr output = scriptedOutput;
    scriptedOutput->setFailure(true);

    try {
        writeOrchestraCue(output);
    } catch (const el::StreamError &error) {
        el::io::printLine("Error: "_el, error.title());
        el::io::printLine("State Failed: "_el, output->state() == el::StreamState::Failed);
    }
}

}
