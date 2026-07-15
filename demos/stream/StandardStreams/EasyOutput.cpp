// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

#include <exception>

namespace demo {

void announceRehearsalPlan();

void writeStandardOutput() {
    announceRehearsalPlan();
}

/// Write concise standard output with a bounded failure policy.
/// The `io` helpers resolve the active `stdOut()` target for every call. Each call is atomic, so timeout means that
/// none of that call was accepted. Flush after the final write when delivery must complete before continuing.
void announceRehearsalPlan() {
    try {
        if (el::io::printLine("Prova d'orchestra — tempo iniziale: "_el, 88, " bpm"_el).isTimeout() ||
            el::io::writeLine("Accelerando dalla battuta 17."_el).isTimeout()) {
            throw el::RuntimeError{"The rehearsal announcement timed out."_el};
        }
        if (el::stdOut()->flush().isTimeout()) {
            throw el::RuntimeError{"Sending the rehearsal program timed out."_el};
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The rehearsal program could not be written."_el, std::current_exception()};
    }
}

}
