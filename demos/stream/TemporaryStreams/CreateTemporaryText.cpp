// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Create a temporary text stream beneath an explicit directory.
/// Temporary-file options control naming, access, and cleanup; text options independently control encoding.
void createTemporaryText() {
    auto temporaryOptions = el::PathTempFileOptions{};
    temporaryOptions.setPrefix("θηλαστικά-"_el).setSuffix(".txt"_el).setAccessProfile(el::PathAccessProfile::UserOnly);
    auto textOptions = el::PathWriteTextOptions{el::StringEncoding::Utf8};

    const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow(
        temporaryOptions, textOptions);
    output->writeLine("Παρατήρηση: δύο ελάφια στο ξέφωτο"_el);
    el::io::printLine("Temporary text with a .txt suffix: "_el, output->path().suffix() == ".txt"_el);
    output->close();
}

}
