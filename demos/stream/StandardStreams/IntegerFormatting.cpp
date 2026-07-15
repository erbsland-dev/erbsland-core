// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StandardStreamsDemos.hpp"

#include <StreamDemoSupport.hpp>

#include <exception>
#include <utility>

namespace demo {

[[nodiscard]] auto readTempoFrom(el::TextInputStreamPtr input) -> el::String;

void readRedirectedInput() {
    const auto directory = createStreamDemoDirectory("tempo"_el);
    const auto path = directory->path() / "input.txt"_el;
    path.content().writeTextOrThrow("Andante\n"_el);
    const auto input = path.content().openTextInputStream();
    el::io::print("Read tempo: "_el, readTempoFrom(input));
    if (el::stdOut()->flush().isTimeout()) {
        throw el::RuntimeError{"Sending the read tempo timed out."_el};
    }
}

/// Redirect standard input to a supplied text stream for deterministic parsing or tests.
/// A stable proxy follows the replacement even when obtained before the redirect.
/// A finite line length and attempt count bound both memory use and time spent on a source that repeatedly stalls.
auto readTempoFrom(el::TextInputStreamPtr input) -> el::String {
    constexpr auto cMaximumLineLength = el::CpLength{40U};
    constexpr auto cMaximumAttempts = 3U;
    const auto standardInput = el::stdIn();

    try {
        auto redirect = el::redirectStdIn(std::move(input));
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            const auto result = standardInput->readLine(cMaximumLineLength);
            if (result.hasData()) {
                return result.data();
            }
            if (result.isFinished()) {
                throw el::RuntimeError{"Standard input does not contain a tempo marking."_el};
            }
        }
        throw el::RuntimeError{"Too many timeouts occurred while reading the tempo."_el};
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The tempo could not be read from standard input."_el, std::current_exception()};
    }
}

}
