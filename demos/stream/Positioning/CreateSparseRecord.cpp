// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Position beyond the current end before writing to create a sparse region where the platform supports it.
/// The logical output position advances as soon as the complete write is accepted, even while bytes remain queued.
void createSparseRecord() {
    const auto directory = createStreamDemoDirectory("稀疏"_el);
    const auto path = directory->path() / "habitat.bin"_el;
    const auto output = path.content().openByteOutputStream();
    output->setPosition(el::ByteIndex{8U});
    output->writeUInt8(42U);
    el::io::printLine("Logical position after acceptance: "_el, output->position().toSizeT());
    output->close();
    el::io::printLine("File length: "_el, path.content().readDataOrThrow().length().toSizeT());
}

}
