// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringTrimTools.hpp"

namespace erbsland::text::impl {

auto U16StringTrimTools::trimmedRange(const Side side) const noexcept -> unit::U16DataRange {
    return trimmedRangeFor(side, [](const Char character) noexcept -> bool { return character.isAsciiWhitespace(); });
}

auto U16StringTrimTools::trimmedRange(const CharSet &characters, const Side side) const noexcept -> unit::U16DataRange {
    return trimmedRangeFor(side, [&](const Char character) noexcept -> bool { return characters.contains(character); });
}

auto U16StringTrimTools::toAbsoluteRange(const unit::U16DataIndex begin, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataRange {
    if (begin >= end) {
        return unit::U16DataRange::empty();
    }
    return unit::U16DataRange{begin, end}.withOrigin(_data.range().index());
}

}
