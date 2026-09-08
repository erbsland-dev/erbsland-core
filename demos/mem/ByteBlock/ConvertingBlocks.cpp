// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Convert a byte block at an interoperability boundary.
///
/// `toByteBuffer()` creates an independent mutable Erbsland Core buffer. The
/// vector conversions copy bytes into standard-library containers for APIs that
/// specifically require unsigned-byte or character storage.
void convertingBlocks() {
    const auto encodedBranch = el::ByteBlock{el::Byte{0x41U}, el::Byte{0x72U}, el::Byte{0x63U}};

    // Create independent containers suited to each receiving API.
    auto mutableBuffer = encodedBranch.toByteBuffer();
    const auto unsignedBytes = encodedBranch.toUInt8Vector();
    const auto characterBytes = encodedBranch.toCharVector();
    mutableBuffer.set(el::ByteIndex{0U}, el::Byte{0x61U});

    el::io::printLine("Skill branch       : Arc ancien"_el);
    el::io::printLine("Original block     : "_el, el::ByteFormat::separated(), encodedBranch);
    el::io::printLine(
        "Mutable buffer     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(mutableBuffer.span()));
    el::io::printLine("Unsigned vector len: "_el, unsignedBytes.size());
    el::io::printLine("Character vector len: "_el, characterBytes.size());
}

}
