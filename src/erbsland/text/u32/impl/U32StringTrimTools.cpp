// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringTrimTools.hpp"

namespace erbsland::text::impl {

auto U32StringTrimTools::trimmedRange(const Side side) const noexcept -> unit::CpRange {
    return trimmedRangeFor(side, [](const Char character) noexcept -> bool { return character.isAsciiWhitespace(); });
}

auto U32StringTrimTools::trimmedRange(const CharSet &characters, const Side side) const noexcept -> unit::CpRange {
    return trimmedRangeFor(side, [&](const Char character) noexcept -> bool { return characters.contains(character); });
}

auto U32StringTrimTools::toAbsoluteRange(const unit::CpIndex begin, const unit::CpIndex end) const noexcept
    -> unit::CpRange {
    if (begin >= end) {
        return unit::CpRange::empty();
    }
    return unit::CpRange{begin, end}.withOrigin(_data.range().index());
}

}
