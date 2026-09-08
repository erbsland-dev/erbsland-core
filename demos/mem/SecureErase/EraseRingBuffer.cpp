// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Securely discard queued bytes from ring buffers.
///
/// Sensitive mode erases bytes as they are consumed or discarded. An explicit
/// `secureErase()` overwrites the complete ring allocation immediately and
/// leaves the buffer empty without changing its capacity.
void eraseRingBuffer() {
    auto records = el::ByteRingBuffer{el::ByteLength{8U}};
    records.setSensitive(true);
    const auto writeResult = records.writeInteger<uint32_t>(0x4e454f31U);
    const auto capacityBeforeErase = records.capacity();
    const auto lengthBeforeErase = records.length();

    // End the protocol session by erasing all queued and unused storage.
    records.secureErase();

    el::io::printLine("Kø               : Asteroide-poster"_el);
    el::io::printLine("Record accepted   : "_el, el::BooleanFormat::yesNo(), writeResult.isSuccessful());
    el::io::printLine("Bytes before erase: "_el, lengthBeforeErase.toSizeT());
    el::io::printLine("Queue empty       : "_el, el::BooleanFormat::yesNo(), records.isEmpty());
    el::io::printLine("Capacity preserved: "_el, el::BooleanFormat::yesNo(), records.capacity() == capacityBeforeErase);
    el::io::printLine("Sensitive mode    : "_el, el::BooleanFormat::yesNo(), records.isSensitive());
}

}
