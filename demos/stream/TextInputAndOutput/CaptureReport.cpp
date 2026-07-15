// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

[[nodiscard]] auto buildPlantSummary() -> el::String;

void captureReport() {
    const auto report = buildPlantSummary();
    el::io::printLine("Captured report:"_el);
    el::io::print(report);
}

/// Produce a complete summary as one atomic text-stream request.
/// Returning the status lets a file or terminal caller apply its own retry policy without coupling this producer to
/// a particular destination.
auto writePlantSummary(el::TextOutputStream &output) -> el::StreamWriteStatus {
    return output.print("Art: fjällsippa\nNya blad: "_el, 5, "\n"_el);
}

/// Capture text-stream output in memory and move the completed string out of the builder.
/// `StringBuilderStream` is always ready and performs no external I/O, so this operation needs no timeout retry.
/// The same stream-oriented writer can also target a file or terminal.
auto buildPlantSummary() -> el::String {
    const auto builder = el::StringBuilderStream::create();
    if (writePlantSummary(*builder).isTimeout()) {
        throw el::LogicError{"An in-memory string builder unexpectedly timed out."};
    }

    return builder->takeString();
}

}
