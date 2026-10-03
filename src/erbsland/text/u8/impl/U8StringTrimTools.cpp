// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringTrimTools.hpp"

namespace erbsland::text::impl {

using namespace unit;

auto U8StringTrimTools::trimmedRange(const Side side) const noexcept -> ByteRange {
    return trimmedRangeFor(side, [](const Char character) noexcept -> bool { return character.isAsciiWhitespace(); });
}

auto U8StringTrimTools::trimmedRange(const CharSet &characters, const Side side) const noexcept -> ByteRange {
    return trimmedRangeFor(side, [&](const Char character) noexcept -> bool { return characters.contains(character); });
}

auto U8StringTrimTools::toAbsoluteRange(const ByteIndex begin, const ByteIndex end) const noexcept -> ByteRange {
    if (begin >= end) {
        return ByteRange::empty();
    }
    return ByteRange{begin, end}.toAbsolute(_data.range().beginIndex());
}

}
