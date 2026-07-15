// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

/// Write one orchestra score into a stream that remains owned by the caller.
/// This helper only writes data. The caller keeps responsibility for closing or aborting the shared stream.
void writeSlowOrchestraScore(const el::ByteOutputStreamPtr &output) {
    output->writeUInt8(88U);
}

/// Continue one graceful close across bounded calls when accepted output must be preserved.
/// `Timeout` leaves the stream in `Closing`; repeat `close()` without writing again. An attempt limit bounds the
/// workflow, and `abort()` provides a non-blocking exit when the destination never completes.
void continueCloseAfterTimeout() {
    constexpr auto cMaximumCloseAttempts = 3U;
    const el::ByteOutputStreamPtr output = std::make_shared<ScriptedByteOutputStream>(0U, 0U, 1U);
    writeSlowOrchestraScore(output);

    try {
        for (auto attempt = 1U; attempt <= cMaximumCloseAttempts; ++attempt) {
            if (output->close().isClosed()) {
                el::io::printLine("Score closed after "_el, attempt, " attempts."_el);
                return;
            }
            el::io::printLine("Close is still in progress; check cancellation and progress."_el);
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The orchestra score could not be closed."_el, std::current_exception()};
    }

    output->abort();
    throw el::RuntimeError{"The orchestra score remained in closing for too long."_el};
}

}
