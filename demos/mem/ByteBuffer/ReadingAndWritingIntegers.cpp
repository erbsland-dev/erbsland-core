// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Read and write fixed-width integers inside a byte buffer.
///
/// Integer access uses an explicit byte offset and byte order. Tolerant methods
/// report an invalid range without changing the buffer, while `OrThrow` methods
/// make a required binary layout easy to enforce.
void readingAndWritingIntegers() {
    auto observation = el::ByteBuffer{el::ByteLength{8U}};

    // Store a big-endian plot identifier and a little-endian tree count.
    observation.setIntegerOrThrow<uint32_t>(el::ByteIndex{0U}, 0x464f5245U, el::Endianness::Big);
    const auto countStored = observation.setInteger<uint16_t>(el::ByteIndex{4U}, 317U, el::Endianness::Little);
    const auto outsideStored = observation.setInteger<uint32_t>(el::ByteIndex{7U}, 1U);

    // Decode fields using the same byte order as their wire representation.
    const auto plotId = observation.getIntegerOrThrow<uint32_t>(el::ByteIndex{0U}, el::Endianness::Big);
    auto treeCount = uint16_t{};
    const auto countRead = observation.getIntegerInto(treeCount, el::ByteIndex{4U}, el::Endianness::Little);

    el::io::printLine("Kayıt             : Orman sayımı"_el);
    el::io::printLine(
        "Encoded record    : "_el, el::ByteFormat::separated(), el::ByteBlock::fromSpan(observation.span()));
    el::io::printLine("Plot identifier   : "_el, plotId);
    el::io::printLine("Tree count        : "_el, treeCount);
    el::io::printLine("Count stored/read : "_el, el::BooleanFormat::yesNo(), countStored && countRead);
    el::io::printLine("Outside write     : "_el, el::BooleanFormat::yesNo(), outsideStored);
}

}
