// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "impl/U16StringTrimTools.hpp"

namespace erbsland::text {

namespace {

using TrimSide = impl::U16StringTrimTools::Side;

auto trimSideFrom(const std::optional<StringSide> side) noexcept -> TrimSide {
    if (!side.has_value()) {
        return TrimSide::Both;
    }
    return *side == StringSide::Front ? TrimSide::Begin : TrimSide::End;
}

}

auto U16String::trim(const std::optional<CharSet> &characters, const std::optional<StringSide> side) -> U16String & {
    const auto trimSide = trimSideFrom(side);
    if (characters.has_value()) {
        *this = withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    } else {
        *this = withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(trimSide));
    }
    return *this;
}

auto U16String::trimmed(const std::optional<CharSet> &characters, const std::optional<StringSide> side) const
    -> U16String {
    const auto trimSide = trimSideFrom(side);
    if (characters.has_value()) {
        return withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    }
    return withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(trimSide));
}

}
