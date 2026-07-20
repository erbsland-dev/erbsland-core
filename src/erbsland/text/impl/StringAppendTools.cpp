// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringAppendTools.hpp"

#include "ThrowHelper.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u8/impl/U8Encoding.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/U16DataIndex.hpp"

namespace erbsland::text::impl {

auto StringAppendTools::countDecodedCharacters(const std::span<const char> source) noexcept -> unit::CpLength {
    auto position = unit::ByteIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < source.size()) {
        utf8::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto StringAppendTools::countDecodedCharacters(const std::span<const char16_t> source) noexcept -> unit::CpLength {
    auto position = unit::U16DataIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < source.size()) {
        utf16::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto StringAppendTools::countDecodedCharacters(const std::span<const char32_t> source) noexcept -> unit::CpLength {
    auto position = unit::CpIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < source.size()) {
        utf32::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto StringAppendTools::repeatedCharacterCount(const unit::CpLength characterCount, const std::size_t count)
    -> unit::CpLength {
    if (characterCount.isZero() || count == 0U) {
        return unit::CpLength::zero();
    }
    const auto characterCountSize = characterCount.toSizeTOrThrow();
    if (count > unit::CpLength::maximum().toSizeT() / characterCountSize) {
        throwOverflow("Repeated string exceeds character length bounds");
    }
    return unit::CpLength::fromSizeTOrThrow(characterCountSize * count);
}

}
