// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Open file-backed streams through `Path::content()`.
/// The path factory returns a shared stream with the platform handle, buffering, encoding, and error context already
/// connected. Always finish owned output with `close()` so queued data reaches the file.
void openFileStreams() {
    const auto directory = createStreamDemoDirectory("skizzen"_el);
    const auto path = directory->path() / "studie.txt"_el;

    // Write a small UTF-8 sketchbook entry.
    const auto output = path.content().openTextOutputStream();
    output->writeLine("Studie 17: Licht auf dem Nordhang"_el);
    output->close();

    // Open the same path for decoded text input.
    const auto input = path.content().openTextInputStream();
    const auto result = input->readAll();
    if (result.hasData()) {
        el::io::print(result.data());
    }
}

}
