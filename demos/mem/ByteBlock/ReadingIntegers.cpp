// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <cstdint>

namespace demo {

/// Decode native integers from known positions in a byte block.
///
/// Integer access can return a fallback, report success while preserving an
/// existing destination on failure, or throw when the binary layout guarantees
/// that the complete integer is present.
void readingIntegers() {
    const auto record = el::ByteBlock{
        el::Byte{0x34U},
        el::Byte{0x12U},
        el::Byte{0x00U},
        el::Byte{0x00U},
        el::Byte{0x00U},
        el::Byte{0x00U},
        el::Byte{0x01U},
        el::Byte{0x2cU}};

    // Read a little-endian rank and a big-endian experience value.
    const auto rank = record.getIntegerOrThrow<uint16_t>(el::ByteIndex{0U}, el::Endianness::Little);
    const auto experience = record.getIntegerOrThrow<uint32_t>(el::ByteIndex{4U}, el::Endianness::Big);

    // Preserve an existing value when an optional field is incomplete.
    auto prestige = uint32_t{7U};
    const auto hasPrestige = record.getIntegerInto(prestige, el::ByteIndex{7U}, el::Endianness::Big);
    const auto fallback = record.getInteger<uint32_t>(el::ByteIndex{7U}, el::Endianness::Big, uint32_t{99U});

    el::io::printLine("Character          : Éclaireuse lunaire"_el);
    el::io::printLine("Rank               : "_el, rank);
    el::io::printLine("Experience         : "_el, experience);
    el::io::printLine("Prestige available : "_el, el::BooleanFormat::yesNo(), hasPrestige);
    el::io::printLine("Preserved prestige : "_el, prestige);
    el::io::printLine("Fallback prestige  : "_el, fallback);
}

}
