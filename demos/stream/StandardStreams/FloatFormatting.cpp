// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

namespace demo {

[[nodiscard]] auto captureRehearsalNotes() -> el::String;

void captureOutput() {
    el::io::printLine("Captured output:"_el);
    el::io::print(captureRehearsalNotes());
    if (el::stdOut()->flush().isTimeout()) {
        throw el::RuntimeError{"Sending the captured output timed out."_el};
    }
}

/// Capture output from code that writes to the standard-output proxy.
/// Keep the redirect guard in a narrow scope so automatic restoration also covers early returns and exceptions.
/// A `AnyStringBuilderStream` performs no external I/O, therefore timeout would violate an in-memory stream invariant.
auto captureRehearsalNotes() -> el::String {
    const auto capture = el::AnyStringBuilderStream::create();
    {
        auto redirect = el::redirectStdOut(capture);
        if (el::io::printLine("Moderato, 96 bpm"_el).isTimeout() ||
            el::io::printLine("Rallentando nelle ultime quattro battute"_el).isTimeout()) {
            throw el::LogicError{"An in-memory standard-output capture unexpectedly timed out."};
        }
    }
    return capture->takeString();
}

}
