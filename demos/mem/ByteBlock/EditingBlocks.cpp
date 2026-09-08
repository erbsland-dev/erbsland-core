// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Assemble and edit a small binary record in place.
///
/// `ByteBlockEditor` combines growing operations such as `append()` and
/// `insert()` with non-growing `overwrite()` and `fill()`, structural changes,
/// and whole-block XOR for compact binary transformations.
void editingBlocks() {
    auto build = el::ByteBlockEditor{el::Byte{0x10U}, el::Byte{0x20U}};

    // Assemble the skill record from bytes, a block, and a fixed-width integer.
    build.append(el::Byte{0x30U})
        .insert(el::ByteIndex{1U}, el::ByteBlock{el::Byte{0x15U}})
        .append(el::ByteBlock{el::Byte{0x40U}, el::Byte{0x50U}})
        .appendInteger(uint16_t{0x1234U}, el::Endianness::Big);

    // Adjust selected fields without changing the overall size.
    build.replace(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{1U}}, el::ByteBlock{el::Byte{0x22U}});
    build.overwrite(el::ByteIndex{3U}, el::ByteArray{el::Byte{0x33U}, el::Byte{0x44U}}.span());
    build.fill(el::ByteRange{el::ByteIndex{5U}, el::ByteLength{1U}}, el::Byte{0x55U});

    // Remove a retired node, keep the record payload, and apply an equal-size mask.
    build.remove(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{1U}});
    build.keep(el::ByteRange{el::ByteIndex{0U}, el::ByteLength{6U}});
    build.xorWithOrThrow(el::ByteBlock{el::ByteLength{6U}, el::Byte{0x0fU}});

    el::io::printLine("Skill build        : Gardien des étoiles"_el);
    el::io::printLine("Encoded build      : "_el, el::ByteFormat::separated(), el::ByteBlock{build});
}

}
