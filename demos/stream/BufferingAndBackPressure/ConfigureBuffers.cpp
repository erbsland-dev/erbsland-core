// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

void openCellImagingStreams(const el::Path &sourcePath, const el::Path &targetPath);

void configureBuffers() {
    const auto directory = createStreamDemoDirectory("μικροσκόπιο"_el);
    const auto sourcePath = directory->path() / "παρατηρήσεις.txt"_el;
    sourcePath.content().writeTextOrThrow("Πυρήνας: ορατός\n"_el);
    openCellImagingStreams(sourcePath, directory->path() / "ανάλυση.txt"_el);
}

/// Configure buffering intentions before opening text streams.
/// Text streams use the settings of their backing byte streams, while the library selects suitable internal sizes for
/// each buffer role.
void openCellImagingStreams(const el::Path &sourcePath, const el::Path &targetPath) {
    auto inputSettings = el::InputStreamSettings{};
    inputSettings.setBuffering(el::StreamBuffering::Interactive);
    auto readOptions = el::PathReadTextOptions{};
    readOptions.setStreamSettings(inputSettings);
    const auto input = sourcePath.content().openTextInputStream(readOptions);

    auto outputSettings = el::OutputStreamSettings{};
    outputSettings.setBuffering(el::StreamBuffering::Throughput).setBackBufferLimit(el::ByteLength{32U * 1024U});
    auto writeOptions = el::PathWriteTextOptions{};
    writeOptions.setStreamSettings(outputSettings);
    const auto output = targetPath.content().openTextOutputStream(writeOptions);

    el::io::printLine("Input buffering: interactive"_el);
    el::io::printLine("Output buffering: throughput"_el);
    el::io::printLine("Output back buffer: "_el, output->outputSettings().backBufferLimit().toSizeT(), " bytes"_el);

    input->close();
    output->close();
}

}
