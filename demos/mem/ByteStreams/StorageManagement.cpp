// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Prepare, transfer, and reuse a byte writer's storage.
///
/// Reserve once when a useful upper bound is known. `takeByteBlockEditor()`
/// transfers editable output, and `reset()` discards bytes before reuse.
void storageManagement() {
    auto writer = el::ByteWriter{};

    // Reserve once for the expected record, then transfer the finished editor.
    writer.reserve(el::ByteLength{64U});
    writer.writeByte(el::Byte{0x4eU}).writeByte(el::Byte{0x01U});
    auto editableRecord = writer.takeByteBlockEditor();

    // Reuse the empty writer for another record and explicitly discard it.
    writer.writeByte(el::Byte{0x4eU});
    writer.reset();
    el::io::printLine("Transferred bytes : "_el, editableRecord.length().toSizeT());
    el::io::printLine("Writer is empty   : "_el, el::BooleanFormat::yesNo(), writer.length().isZero());
}

}
