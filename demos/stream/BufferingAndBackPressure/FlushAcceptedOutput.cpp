// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

[[nodiscard]] auto askForCellAtlas() -> el::String;

void flushAcceptedOutput() {
    const auto directory = createStreamDemoDirectory("είσοδος"_el);
    const auto path = directory->path() / "απάντηση.txt"_el;
    path.content().writeTextOrThrow("Άτλας βρύων\n"_el);
    const auto input = path.content().openTextInputStream();
    auto redirect = el::redirectStdIn(input);

    el::io::print(askForCellAtlas());
}

/// Flush a prompt before waiting for standard input.
/// A successful write only places the prompt in the output queue. `flush()` waits for accepted text to reach the
/// terminal, so the user can see the question before the program starts reading the answer.
auto askForCellAtlas() -> el::String {
    constexpr auto cMaximumAttempts = 3U;
    constexpr auto cMaximumAnswerLength = el::CpLength{80U};
    const auto output = el::stdOut();

    if (output->write("Cell atlas name: "_el).isTimeout()) {
        throw el::RuntimeError{"The prompt was not accepted in time."_el};
    }

    for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
        if (output->flush().isSuccess()) {
            const auto result = el::stdIn()->readLine(cMaximumAnswerLength);
            if (result.hasData()) {
                return result.data();
            }
            if (result.isFinished()) {
                throw el::RuntimeError{"No atlas name was provided."_el};
            }
        }
    }
    throw el::RuntimeError{"The interaction did not finish in time."_el};
}

}
