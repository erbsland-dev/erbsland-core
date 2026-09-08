// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Manage mutable byte-block length, capacity, and shared storage.
///
/// Reserving avoids repeated growth while assembling data. Resizing changes the
/// visible length, `detach()` eagerly establishes unique storage, and clearing or
/// resetting chooses whether allocated capacity is retained.
void managingStorage() {
    auto path = el::ByteBlockEditor{el::Byte{1U}, el::Byte{2U}, el::Byte{3U}};

    // Reserve working room and grow the visible block with zero-filled bytes.
    path.reserve(el::ByteLength{32U});
    const auto reservedCapacity = path.capacity();
    path.resize(el::ByteLength{6U});

    // Preserve a read-only snapshot and detach before a sequence of edits.
    const auto snapshot = el::ByteBlock{path};
    path.detach();
    path.fill(el::ByteRange{el::ByteIndex{3U}, el::ByteLength{3U}}, el::Byte{9U});
    path.shrinkToFit();

    el::io::printLine("Skill path         : Discipline astrale"_el);
    el::io::printLine("Reserved capacity  : "_el, reservedCapacity.toSizeT());
    el::io::printLine("Edited bytes       : "_el, el::ByteFormat::separated(), el::ByteBlock{path});
    el::io::printLine("Snapshot bytes     : "_el, el::ByteFormat::separated(), snapshot);
    el::io::printLine("Tight capacity     : "_el, path.capacity().toSizeT());

    // Clear retains the tight allocation; reset releases it.
    path.clear();
    el::io::printLine("Capacity after clear: "_el, path.capacity().toSizeT());
    path.reset();
    el::io::printLine("Capacity after reset: "_el, path.capacity().toSizeT());
}

}
