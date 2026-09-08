// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>

namespace demo {

/// Securely erase fixed byte storage and a borrowed writable span.
///
/// `ByteArray::secureErase()` handles the complete array. The free
/// `secureErase()` function applies the same platform-backed operation to any
/// writable `ByteSpan` without taking ownership of its storage.
void eraseFixedStorage() {
    auto authenticationTag = el::ByteArray{
        el::Byte{0x41U},
        el::Byte{0x70U},
        el::Byte{0x6fU},
        el::Byte{0x66U},
        el::Byte{0x69U},
        el::Byte{0x73U},
    };
    auto decoderScratch = std::array{
        el::Byte{0x53U},
        el::Byte{0x70U},
        el::Byte{0x65U},
        el::Byte{0x6bU},
        el::Byte{0x74U},
        el::Byte{0x72U},
    };

    // Erase owned fixed storage through its member function.
    authenticationTag.secureErase();
    const auto tagIsZero = authenticationTag == el::ByteArray<6>{};

    // Erase caller-owned storage through a writable borrowed span.
    el::mem::secureErase(el::ByteSpan{decoderScratch});
    const auto scratchIsZero = decoderScratch == std::array<el::Byte, 6>{};

    el::io::printLine("Observation        : Apofis-spektrum"_el);
    el::io::printLine("Tag erased         : "_el, el::BooleanFormat::yesNo(), tagIsZero);
    el::io::printLine("Scratch erased     : "_el, el::BooleanFormat::yesNo(), scratchIsZero);
}

}
