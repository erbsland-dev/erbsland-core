// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void readObservationLines(el::TextInputStream &input);

void readObservationLines() {
    const auto directory = createStreamDemoDirectory("linjer"_el);
    const auto path = directory->path() / "matning.txt"_el;
    path.content().writeTextOrThrow("Dag 1: 2 cm\nDag 8: 7 cm\nDag 15: 13 cm"_el);
    auto options = el::PathReadTextOptions{};
    options.setTimeout(el::TimeDelta::seconds(1));
    const auto input = path.content().openTextInputStream(options);
    readObservationLines(*input);
}

/// Read lines with a finite length and a finite retry policy.
/// `readLine()` keeps a line ending when one is present and returns a final unterminated line as data.
/// Repeating the same call after timeout continues the pending logical line.
void readObservationLines(el::TextInputStream &input) {
    constexpr auto cMaximumLineLength = el::CpLength{40U};
    constexpr auto cMaximumTimeouts = 3U;

    auto lineNumber = 1;
    auto consecutiveTimeouts = 0U;
    try {
        while (true) {
            const auto result = input.readLine(cMaximumLineLength);
            if (result.isFinished()) {
                return;
            }
            if (result.isTimeout()) {
                if (++consecutiveTimeouts == cMaximumTimeouts) {
                    throw el::RuntimeError{"Too many timeouts occurred while reading the measurement series."_el};
                }
                continue;
            }

            consecutiveTimeouts = 0U;
            el::io::print("Line "_el, lineNumber++, ": "_el, result.data());
            if (!result.data().endsWith("\n"_el)) {
                el::io::writeLine();
            }
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The measurement series could not be read."_el, std::current_exception()};
    }
}

}
