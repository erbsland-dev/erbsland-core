// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "impl/U8StringTrimTools.hpp"

namespace erbsland::text {

auto U8String::trimmed(const std::optional<CharSet> &characters, const std::optional<StringSide> side) const
    -> U8String {
    const auto trimSide = impl::U8StringTrimTools::sideFrom(side);
    auto trimTools = impl::U8StringTrimTools{dataView()};
    return withRange(
        characters.has_value() ? trimTools.trimmedRange(*characters, trimSide) : trimTools.trimmedRange(trimSide));
}

}
