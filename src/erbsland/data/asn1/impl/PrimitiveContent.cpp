// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PrimitiveContent.hpp"

namespace erbsland::data::asn1::impl {

auto isMinimalInteger(const mem::ConstByteSpan content) noexcept -> bool {
    if (content.size() < 2U) {
        return !content.empty();
    }
    const auto first = content.front().toUInt8();
    const auto secondIsNegative = (content[1].toUInt8() & 0x80U) != 0U;
    return (first != 0U || secondIsNegative) && (first != 0xffU || !secondIsNegative);
}

auto isValidBitString(const mem::ConstByteSpan content) noexcept -> bool {
    if (content.empty()) {
        return false;
    }
    const auto unused = content.front().toUInt8();
    if (unused == 0U) {
        return true;
    }
    return unused <= 7U && content.size() > 1U && (content.back().toUInt8() & ((1U << unused) - 1U)) == 0U;
}

}
