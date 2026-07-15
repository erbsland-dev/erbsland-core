// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Temporary byte streams provide the regular atomic byte-output interface plus automatic file cleanup.
/// Use them for intermediate binary data that should not survive the owning operation.
void createTemporaryBytes() {
    auto options = el::PathTempFileOptions{};
    options.setPrefix("ίχνη-"_el).setSuffix(".bin"_el);
    const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow(options);
    output->writeUInt16(14U);
    output->writeUInt16(27U);
    el::io::printLine("Binary data: "_el, output->path().suffix() == ".bin"_el);
    output->close();
}

}
