// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Protect and explicitly erase a uniquely owned dynamic byte buffer.
///
/// Sensitive mode erases discarded ranges and replaced allocations throughout
/// the buffer's lifetime. `secureErase()` immediately overwrites its complete
/// capacity while preserving length, capacity, and sensitive mode.
void eraseDynamicBuffer() {
    auto sessionMaterial = el::ByteBuffer{el::ByteLength{6U}};
    sessionMaterial.setSensitive(true);
    sessionMaterial.set(el::ByteIndex{0U}, el::Byte{0x4eU});
    sessionMaterial.set(el::ByteIndex{1U}, el::Byte{0x45U});
    sessionMaterial.set(el::ByteIndex{2U}, el::Byte{0x4fU});
    sessionMaterial.set(el::ByteIndex{3U}, el::Byte{0x2dU});
    sessionMaterial.set(el::ByteIndex{4U}, el::Byte{0x31U});
    sessionMaterial.set(el::ByteIndex{5U}, el::Byte{0x37U});
    sessionMaterial.reserve(el::ByteLength{32U});
    const auto lengthBeforeErase = sessionMaterial.length();
    const auto capacityBeforeErase = sessionMaterial.capacity();

    // Overwrite visible bytes and unused capacity as soon as the session ends.
    sessionMaterial.secureErase();
    const auto zeros = el::ByteArray<6>{};

    el::io::printLine("Session            : NEO-17"_el);
    el::io::printLine(
        "Buffer erased      : "_el, el::BooleanFormat::yesNo(), sessionMaterial.isEqualConstTime(zeros.span()));
    el::io::printLine(
        "Length preserved   : "_el, el::BooleanFormat::yesNo(), sessionMaterial.length() == lengthBeforeErase);
    el::io::printLine(
        "Capacity preserved : "_el, el::BooleanFormat::yesNo(), sessionMaterial.capacity() == capacityBeforeErase);
    el::io::printLine("Sensitive mode     : "_el, el::BooleanFormat::yesNo(), sessionMaterial.isSensitive());
}

}
