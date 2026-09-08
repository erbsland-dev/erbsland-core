// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace demo {

/// Create owning byte blocks from common sources.
///
/// `ByteBlock` is the read-only owning value for storing and passing binary data.
/// `ByteBlockEditor` provides the same construction choices when the bytes must
/// remain mutable, and converts implicitly to a read-only block without copying.
void creatingBlocks() {
    // Create blocks with a repeated value and with individual skill bytes.
    const auto emptySlots = el::ByteBlock{el::ByteLength{4U}, el::Byte{0U}};
    const auto learnedSkills = el::ByteBlock{el::Byte{1U}, el::Byte{3U}, el::Byte{8U}};

    // Copy fixed arrays, borrowed spans, and standard vectors into owned blocks.
    const auto fixedSkills = el::ByteArray{el::Byte{2U}, el::Byte{5U}, el::Byte{13U}};
    const auto networkBytes = std::array<std::byte, 3>{std::byte{21U}, std::byte{34U}, std::byte{55U}};
    const auto savedBytes = std::vector<uint8_t>{89U, 144U};
    const auto fromArray = el::ByteBlock{fixedSkills};
    const auto fromSpan = el::ByteBlock::fromSpan(std::span{networkBytes});
    const auto fromVector = el::ByteBlock::fromVector(savedBytes);

    // Build mutable data and hand it to an API expecting a read-only block.
    auto editor = el::ByteBlockEditor{el::Byte{1U}, el::Byte{2U}};
    editor.append(el::Byte{3U});
    const el::ByteBlock immutableSkills = editor;

    el::io::printLine("Character          : Gardienne des brumes"_el);
    el::io::printLine("Empty slots        : "_el, el::ByteFormat::separated(), emptySlots);
    el::io::printLine("Learned skills     : "_el, el::ByteFormat::separated(), learnedSkills);
    el::io::printLine("From fixed array   : "_el, el::ByteFormat::separated(), fromArray);
    el::io::printLine("From borrowed span : "_el, el::ByteFormat::separated(), fromSpan);
    el::io::printLine("From vector        : "_el, el::ByteFormat::separated(), fromVector);
    el::io::printLine("Editor as block    : "_el, el::ByteFormat::separated(), immutableSkills);
}

}
