// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

void printFileAsHex(const el::ByteInputStreamPtr &input);
void printFileAsText(const el::TextInputStreamPtr &input);

void chooseStreamFamily() {
    const auto directory = createStreamDemoDirectory("ansichten"_el);
    const auto path = directory->path() / "motiv.txt"_el;
    path.content().writeTextOrThrow("Motiv: Öl"_el);

    const auto byteInput = path.content().openByteInputStream();
    printFileAsHex(byteInput);
    byteInput->close();

    const auto textInput = path.content().openTextInputStream();
    printFileAsText(textInput);
    textInput->close();
}

/// Accept a byte input stream when a component must work with the exact file representation.
/// This function can read any byte source; it neither knows nor cares that the caller opened a file.
void printFileAsHex(const el::ByteInputStreamPtr &input) {
    const auto result = input->readAll(el::ByteLength{1024U});
    if (result.hasData()) {
        // The UTF-8 encoding of “Ö” is visible as the final two bytes.
        el::io::printLine("Bytes: "_el, el::ByteFormat::separated(), result.data());
    }
}

/// Accept a text input stream when a component needs decoded Unicode characters.
/// The same function also works with temporary, standard, redirected, or custom text input streams.
void printFileAsText(const el::TextInputStreamPtr &input) {
    const auto result = input->readAll(el::CpLength{1024U});
    if (result.hasData()) {
        // The stream decoder turns the file encoding into an Erbsland Core string.
        el::io::printLine("Text: "_el, result.data());
    }
}

}
