// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringTrimTools.hpp"

namespace erbsland::text::impl {

auto U8StringTrimTools::trimmedRange(const Side side) const noexcept -> unit::ByteRange {
    return trimmedRangeFor(side, [](const Char character) noexcept -> bool { return character.isAsciiWhitespace(); });
}

auto U8StringTrimTools::trimmedRange(const CharSet &characters, const Side side) const noexcept -> unit::ByteRange {
    return trimmedRangeFor(side, [&](const Char character) noexcept -> bool { return characters.contains(character); });
}

auto U8StringTrimTools::toAbsoluteRange(const unit::ByteIndex begin, const unit::ByteIndex end) const noexcept
    -> unit::ByteRange {
    if (begin >= end) {
        return unit::ByteRange::empty();
    }
    return unit::ByteRange{begin, end}.withOrigin(_data.range().index());
}

}
