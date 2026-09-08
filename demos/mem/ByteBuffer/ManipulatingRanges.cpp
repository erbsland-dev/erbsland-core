// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Assemble and revise ranges in a reusable byte buffer.
///
/// Non-structural operations modify bytes already present. `append()`,
/// `insert()`, `replace()`, `remove()`, and `keep()` change the visible layout,
/// making the buffer suitable for repeatedly building binary records.
void manipulatingRanges() {
    auto habitat = el::ByteBuffer{el::ByteLength{6U}, el::Byte{10U}};

    // Modify existing survey fields without changing the length.
    habitat.fill(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{2U}}, el::Byte{20U});
    habitat.overwrite(el::ByteIndex{3U}, el::ByteArray{el::Byte{30U}, el::Byte{40U}}.span());
    habitat.xorWith(
        el::ByteRange{el::ByteIndex{0U}, el::ByteLength{3U}},
        el::ByteArray{el::Byte{1U}, el::Byte{2U}, el::Byte{4U}}.span());

    // Change the record structure, then retain only the useful payload.
    habitat.insert(el::ByteIndex{2U}, el::ByteArray{el::Byte{15U}}.span());
    habitat.replace(
        el::ByteRange{el::ByteIndex{4U}, el::ByteLength{2U}},
        el::ByteArray{el::Byte{33U}, el::Byte{44U}, el::Byte{55U}}.span());
    habitat.remove(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{1U}});
    habitat.append(el::Byte{99U});
    habitat.keep(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{6U}});

    el::io::printLine("Habitat           : Kayın korusu"_el);
    el::io::printLine("Final record      : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(habitat.span()));
    el::io::printLine("Record length     : "_el, habitat.length().toSizeT());
}

}
