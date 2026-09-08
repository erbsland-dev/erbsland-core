// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Follow FIFO order through wrap-around and automatic growth.
///
/// Reads consume the oldest bytes. Later writes may wrap around the allocation;
/// a growable ring can reorganize that queued data into a larger allocation
/// without changing the order observed by readers.
void understandingGrowth() {
    auto samples = el::RingBuffer{el::ByteLength{4U}, el::ByteLength{8U}};

    // Fill most of the initial storage and consume the two oldest samples.
    const auto firstWrite = samples.write(el::ByteArray{el::Byte{1U}, el::Byte{2U}, el::Byte{3U}}.span());
    const auto firstRead = samples.read(el::ByteLength{2U});

    // This write wraps and then grows the ring, while preserving FIFO order.
    const auto secondWrite =
        samples.write(el::ByteArray{el::Byte{4U}, el::Byte{5U}, el::Byte{6U}, el::Byte{7U}}.span());
    const auto capacityAfterGrowth = samples.capacity();
    const auto remaining = samples.read(el::ByteLength::infinite());

    el::io::printLine("Comunità          : Coralli e castagnole"_el);
    el::io::printLine("First write/read  : "_el, firstWrite.toSizeT(), " / "_el, firstRead.length().toSizeT());
    el::io::printLine("Second write      : "_el, secondWrite.toSizeT());
    el::io::printLine("Grown capacity    : "_el, capacityAfterGrowth.toSizeT());
    el::io::printLine("FIFO result       : "_el, el::ByteFormat::separated(), remaining);
}

}
