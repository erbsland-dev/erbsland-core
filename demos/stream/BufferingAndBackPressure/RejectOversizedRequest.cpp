// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

void writeOversizedCellNote(const el::Path &path);

void rejectOversizedRequest() {
    const auto directory = createStreamDemoDirectory("όριο"_el);

    try {
        writeOversizedCellNote(directory->path() / "κύτταρα.txt"_el);
    } catch (const el::StreamError &error) {
        el::io::printLine(error.title());
        el::io::printLine(error.description());
    }
}

/// Keep every atomic text write within the stream's back-buffer limit.
/// The limit applies to encoded bytes. A request that exceeds it fails before any text is accepted, even when the front
/// buffer is empty.
void writeOversizedCellNote(const el::Path &path) {
    auto settings = el::OutputStreamSettings{};
    settings.setBuffering(el::StreamBuffering::MinimalMemory).setBackBufferLimit(el::ByteLength{16U});
    auto options = el::PathWriteTextOptions{};
    options.setStreamSettings(settings);
    const auto output = path.content().openTextOutputStream(options);

    output->writeLine("Παρατήρηση: η κυτταρική μεμβράνη παραμένει ακέραιη."_el);
}

}
