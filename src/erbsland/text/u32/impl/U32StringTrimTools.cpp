// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringTrimTools.hpp"

namespace erbsland::text::impl {

using namespace unit;

auto U32StringTrimTools::trimmedRange(const Side side) const noexcept -> CpRange {
    return trimmedRangeFor(side, [](const Char character) noexcept -> bool { return character.isAsciiWhitespace(); });
}

auto U32StringTrimTools::trimmedRange(const CharSet &characters, const Side side) const noexcept -> CpRange {
    return trimmedRangeFor(side, [&](const Char character) noexcept -> bool { return characters.contains(character); });
}

auto U32StringTrimTools::toAbsoluteRange(const CpIndex begin, const CpIndex end) const noexcept -> CpRange {
    if (begin >= end) {
        return CpRange::empty();
    }
    return CpRange{begin, end}.toAbsolute(_data.range().beginIndex());
}

}
