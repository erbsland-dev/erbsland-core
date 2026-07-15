// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

void readCellObservationLog(el::TextInputStream &input);

void readBoundedChunks() {
    const auto directory = createStreamDemoDirectory("παρατηρήσεις"_el);
    const auto path = directory->path() / "κύτταρα.txt"_el;
    path.content().writeTextOrThrow(
        "Δείγμα 1: πυρήνας ορατός\n"
        "Δείγμα 2: κυτταρική διαίρεση\n"
        "Δείγμα 3: μεμβράνη ακέραιη\n"_el);
    const auto input = path.content().openTextInputStream();
    readCellObservationLog(*input);
}

/// Read a text file line by line without assembling partial input in application code.
/// `readLine()` retains an incomplete line after a timeout. Retry the same call and the stream continues collecting the
/// line until it can return it as one value.
void readCellObservationLog(el::TextInputStream &input) {
    constexpr auto cMaximumLineLength = el::CpLength{80U};
    constexpr auto cMaximumTimeouts = 3U;
    auto consecutiveTimeouts = 0U;

    while (true) {
        const auto result = input.readLine(cMaximumLineLength);
        if (result.hasData()) {
            consecutiveTimeouts = 0U;
            el::io::print(result.data());
            continue;
        }
        if (result.isFinished()) {
            return;
        }
        if (++consecutiveTimeouts == cMaximumTimeouts) {
            throw el::RuntimeError{"Reading the observations timed out repeatedly."_el};
        }
    }
}

}
