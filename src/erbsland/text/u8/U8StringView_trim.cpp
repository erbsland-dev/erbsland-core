// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringView.hpp"

#include "impl/U8StringTrimTools.hpp"

namespace erbsland::text {

namespace {

using TrimSide = impl::U8StringTrimTools::Side;

}

namespace {

auto trimSideFrom(const std::optional<StringSide> side) noexcept -> TrimSide {
    if (!side.has_value()) {
        return TrimSide::Both;
    }
    return *side == StringSide::Front ? TrimSide::Begin : TrimSide::End;
}

}

auto U8StringView::trimmed(const std::optional<CharSet> &characters, const std::optional<StringSide> side) const
    -> U8StringView {
    const auto trimSide = trimSideFrom(side);
    auto trimTools = impl::U8StringTrimTools{dataView()};
    return withRange(
        characters.has_value() ? trimTools.trimmedRange(*characters, trimSide) : trimTools.trimmedRange(trimSide));
}

}
