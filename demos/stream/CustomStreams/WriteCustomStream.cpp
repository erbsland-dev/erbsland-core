// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SkillTreeStreams.hpp"

namespace demo {

/// A custom output stream must accept each write completely or return timeout without accepting anything.
/// Keep settings immutable, make lifecycle transitions explicit, and serialize access when multiple threads can write.
void writeCustomStream() {
    const auto output = SkillTreeOutputStream::create();
    output->setEndianness(el::Endianness::Little);
    output->writeUInt16(12U);
    output->writeUInt16(3U);
    output->flush();

    el::io::printLine("Atomically written bytes: "_el, output->bytes().size());
    output->close();
}

}
