// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "impl/U32StringTrimTools.hpp"

namespace erbsland::text {

auto U32String::trim(const std::optional<CharSet> &characters, const std::optional<StringSide> side) -> U32String & {
    const auto trimSide = impl::U32StringTrimTools::sideFrom(side);
    if (characters.has_value()) {
        *this = withRange(impl::U32StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    } else {
        *this = withRange(impl::U32StringTrimTools{dataView()}.trimmedRange(trimSide));
    }
    return *this;
}

auto U32String::trimmed(const std::optional<CharSet> &characters, const std::optional<StringSide> side) const
    -> U32String {
    const auto trimSide = impl::U32StringTrimTools::sideFrom(side);
    if (characters.has_value()) {
        return withRange(impl::U32StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    }
    return withRange(impl::U32StringTrimTools{dataView()}.trimmedRange(trimSide));
}

}
