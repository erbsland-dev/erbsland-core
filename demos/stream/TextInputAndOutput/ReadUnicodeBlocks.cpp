// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void readUnicodeBlocks(el::TextInputStream &input);

void readUnicodeBlocks() {
    const auto directory = createStreamDemoDirectory("vaxt"_el);
    const auto path = directory->path() / "anteckningar.txt"_el;
    path.content().writeTextOrThrow("Frö 🌱 – två blad – höjd 7 cm"_el);
    auto options = el::PathReadTextOptions{};
    options.setTimeout(el::TimeDelta::seconds(1));
    const auto input = path.content().openTextInputStream(options);
    readUnicodeBlocks(*input);
}

/// Read decoded text in bounded, code-point-safe blocks.
/// Handle normal end, timeout, and stream failure separately; a finite retry limit prevents a stalled source from
/// keeping the application in this loop forever.
void readUnicodeBlocks(el::TextInputStream &input) {
    constexpr auto cBlockLength = el::CpLength{8U};
    constexpr auto cMaximumTimeouts = 3U;
    auto consecutiveTimeouts = 0U;

    try {
        while (true) {
            const auto result = input.read(cBlockLength);
            if (result.isFinished()) {
                return;
            }
            if (result.isTimeout()) {
                if (++consecutiveTimeouts == cMaximumTimeouts) {
                    throw el::RuntimeError{"Too many timeouts occurred while reading the plant notes."_el};
                }
                continue;
            }

            consecutiveTimeouts = 0U;
            el::io::printLine("Block: ["_el, result.data(), "]"_el);
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The plant notes could not be read."_el, std::current_exception()};
    }
}

}
