// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

[[nodiscard]] auto countRiverLines(const el::TextInputStreamPtr &input) -> el::CoTask<std::size_t> {
    auto generator = input->coReadLines();
    auto count = std::size_t{0U};
    while (auto next = co_await generator.next()) {
        if (next->hasData()) {
            ++count;
        }
    }
    co_return count;
}

/// `coReadLines()` combines decoded, code-point-safe line input with asynchronous iteration.
/// It yields complete lines or bounded fragments and completes normally after the final unterminated line.
void processLineGenerator() {
    const auto directory = createStreamDemoDirectory("flod"_el);
    const auto path = directory->path() / "målinger.txt"_el;
    path.content().writeTextOrThrow("Nord: 12 cm\nMidte: 37 cm\nSyd: 19 cm"_el);
    const auto input = path.content().openTextInputStream();
    auto task = countRiverLines(input);
    waitForTask(task);
    el::io::printLine("Measurement lines: "_el, task.result());
}

}
