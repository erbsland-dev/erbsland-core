// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Start a growing `ByteBuffer` with the contents of a `ByteArray`.
///
/// `toByteBuffer()` makes an independent dynamic copy. The resulting buffer can
/// grow as more bytes arrive, while the original fixed-size array remains a
/// compact, unchanged value.
void growingIntoBuffer() {
    const auto initialWave = el::ByteArray{el::Byte{22U}, el::Byte{58U}, el::Byte{104U}, el::Byte{58U}};

    // Begin with a fixed observation, then append a longer measured tail.
    auto recordedWave = initialWave.toByteBuffer();
    recordedWave.append(el::Byte{22U});
    recordedWave.append(el::ByteArray{el::Byte{10U}, el::Byte{4U}}.span());

    el::io::printLine("Wave               : Havsvåg"_el);
    el::io::printLine("Initial samples    : "_el, el::ByteFormat::separated(), el::ByteBlock{initialWave});
    el::io::printLine(
        "Growing buffer     : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(recordedWave.span()));
    el::io::printLine("Initial length     : "_el, initialWave.length().toSizeT());
    el::io::printLine("Buffer length      : "_el, recordedWave.length().toSizeT());
}

}
