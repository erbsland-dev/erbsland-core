// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringTrimTools.hpp"

namespace erbsland::text::impl {

using namespace unit;

auto U16StringTrimTools::trimmedRange(const Side side) const noexcept -> U16Range {
    return trimmedRangeFor(side, [](const Char character) noexcept -> bool { return character.isAsciiWhitespace(); });
}

auto U16StringTrimTools::trimmedRange(const CharSet &characters, const Side side) const noexcept -> U16Range {
    return trimmedRangeFor(side, [&](const Char character) noexcept -> bool { return characters.contains(character); });
}

auto U16StringTrimTools::toAbsoluteRange(const U16Index begin, const U16Index end) const noexcept -> U16Range {
    if (begin >= end) {
        return U16Range::empty();
    }
    return U16Range{begin, end}.toAbsolute(_data.range().beginIndex());
}

}
