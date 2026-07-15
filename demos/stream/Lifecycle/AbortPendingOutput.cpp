// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Write an incomplete orchestra recording into a stream that remains owned by the caller.
/// The helper borrows the shared stream and therefore leaves its lifecycle unchanged for the caller to decide.
void writeIncompleteOrchestraRecording(const el::ByteOutputStreamPtr &output) {
    output->writeUInt32(0x4f524348U);
}

/// Abandon pending output explicitly when cancellation matters more than delivery.
/// `abort()` is non-throwing and returns immediately. Queued data may be lost, so use it only when the incomplete
/// orchestra recording must not be published.
void abortPendingOutput() {
    const auto scriptedOutput = std::make_shared<ScriptedByteOutputStream>();
    const el::ByteOutputStreamPtr output = scriptedOutput;
    writeIncompleteOrchestraRecording(output);

    el::io::printLine("Bytes before abort: "_el, scriptedOutput->bytes().size());
    output->abort();
    el::io::printLine("Stream closed: "_el, output->state() == el::StreamState::Closed);
    el::io::printLine("Bytes after abort: "_el, scriptedOutput->bytes().size());
}

}
