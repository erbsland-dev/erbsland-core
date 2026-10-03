// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringAppendTools.hpp"

#include "RepeatCount.hpp"
#include "ThrowHelper.hpp"

#include "../u16/impl/U16Encoding.hpp"
#include "../u32/impl/U32Encoding.hpp"
#include "../u8/impl/U8Encoding.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/CpCount.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/U16Index.hpp"

namespace erbsland::text::impl {

using namespace unit;

auto StringAppendTools::countDecodedCharacters(const std::span<const char> source) noexcept -> CpCount {
    auto position = ByteIndex::zero();
    auto result = CpCount::zero();
    while (position.toSizeT() < source.size()) {
        utf8::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto StringAppendTools::countDecodedCharacters(const std::span<const char16_t> source) noexcept -> CpCount {
    auto position = U16Index::zero();
    auto result = CpCount::zero();
    while (position.toSizeT() < source.size()) {
        utf16::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto StringAppendTools::countDecodedCharacters(const std::span<const char32_t> source) noexcept -> CpCount {
    auto position = CpIndex::zero();
    auto result = CpCount::zero();
    while (position.toSizeT() < source.size()) {
        utf32::fastAdvanceChar(source, position);
        ++result;
    }
    return result;
}

auto StringAppendTools::repeatedCharacterCount(const CpCount characterCount, const std::size_t count) -> CpCount {
    if (characterCount.isZero() || count == 0U) {
        return CpCount::zero();
    }
    const auto characterCountSize = repeatCountToSize(characterCount);
    if (count > CpCount::maximum().toSizeT() / characterCountSize) {
        throwOutOfRangeFailure("Repeated string exceeds character length bounds");
    }
    return CpCount::fromSizeT(characterCountSize * count);
}

}
