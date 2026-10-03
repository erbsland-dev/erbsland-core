// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/BooleanFormat.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteRingBuffer.hpp>
#include <erbsland/Endianness.hpp>
#include <erbsland/RingBuffer.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

/// Create fixed and growable FIFO byte buffers.
///
/// `RingBuffer` queues raw byte streams, while `ByteRingBuffer` adds atomic
/// integer operations and a configurable byte order. A single capacity creates
/// a fixed ring; two capacities allow growth up to a hard limit.
void creatingBuffers() {
    // Use a fixed ring when the queue must stay within a strict memory budget.
    const auto fixedSamples = el::RingBuffer{el::ByteCount{8U}};

    // Give a growable ring a modest initial allocation and an explicit ceiling.
    const auto growingSamples = el::RingBuffer{el::ByteCount{4U}, el::ByteCount{32U}};

    // Choose ByteRingBuffer when the stream contains structured integer fields.
    auto structuredSamples = el::ByteRingBuffer{el::ByteCount{8U}, el::ByteCount{32U}};
    structuredSamples.setEndianness(el::Endianness::Big);

    el::io::printLine("Barriera          : Secca azzurra"_el);
    el::io::printLine("Fixed capacity    : "_el, fixedSamples.capacity().toSizeT());
    el::io::printLine("Fixed maximum     : "_el, fixedSamples.maximumCapacity().toSizeT());
    el::io::printLine("Growing capacity  : "_el, growingSamples.capacity().toSizeT());
    el::io::printLine("Growing maximum   : "_el, growingSamples.maximumCapacity().toSizeT());
    el::io::printLine(
        "Structured big-endian: "_el,
        el::BooleanFormat::yesNo(),
        structuredSamples.endianness() == el::Endianness::Big);
}

}
