// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

#include <exception>

namespace demo {

void reportRehearsalResult();

void writeStandardError() {
    reportRehearsalResult();
}

/// Keep regular results and human-readable diagnostics on separate standard streams.
/// `stdOut()` and `stdErr()` return stable process-wide proxies. Check each channel independently.
/// A bounded timeout is ordinary flow control, while a failed destination throws `StreamError`.
/// Flush both channels before the command exits.
void reportRehearsalResult() {
    try {
        if (el::stdOut()->printLine("tempo_bpm=88"_el).isTimeout()) {
            throw el::RuntimeError{"Writing the result timed out."_el};
        }
        if (el::stdErr()->printLine("Warning: measure 24 has no dynamic marking."_el).isTimeout()) {
            throw el::RuntimeError{"Writing the warning timed out."_el};
        }
        if (el::stdOut()->flush().isTimeout() || el::stdErr()->flush().isTimeout()) {
            throw el::RuntimeError{"Sending the rehearsal result timed out."_el};
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The rehearsal result could not be published."_el, std::current_exception()};
    }
}

}
