// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>

namespace demo {

/// Write and consume byte sequences with partial or atomic semantics.
///
/// `write()` accepts as many bytes as fit below the hard limit. `writeExact()`
/// either queues a complete sequence or leaves the ring unchanged. Reads likewise
/// consume either into caller-owned storage or into a new `ByteBlock`.
void readingAndWritingBytes() {
    auto current = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{6U}};

    // Queue a complete observation and reject an atomic record that cannot fit.
    const auto first = el::ByteArray{el::Byte{10U}, el::Byte{20U}, el::Byte{30U}, el::Byte{40U}};
    const auto firstWrite = current.write(first.span());
    const auto rejected = current.writeExact(el::ByteArray{el::Byte{50U}, el::Byte{60U}, el::Byte{70U}}.span());

    // Consume two bytes into caller storage, then append a partial sequence.
    auto prefixStorage = std::array<el::Byte, 2>{};
    const auto prefixLength = current.read(el::ByteSpan{prefixStorage});
    const auto partialWrite =
        current.write(el::ByteArray{el::Byte{50U}, el::Byte{60U}, el::Byte{70U}, el::Byte{80U}}.span());
    const auto remaining = current.read(el::ByteLength::infinite());

    el::io::printLine("Corrente          : Canale della barriera"_el);
    el::io::printLine("First write       : "_el, firstWrite.toSizeT());
    el::io::printLine("Atomic write      : "_el, el::BooleanFormat::yesNo(), rejected.isSuccessful());
    el::io::printLine("Consumed prefix   : "_el, prefixLength.toSizeT(), " bytes"_el);
    el::io::printLine("Partial write     : "_el, partialWrite.toSizeT());
    el::io::printLine("Remaining FIFO    : "_el, el::ByteFormat::separated(), remaining);
}

}
