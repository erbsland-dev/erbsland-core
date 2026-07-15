// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringCharView.hpp"

#include "U16StringView.hpp"

#include "impl/U16StringTrimTools.hpp"

namespace erbsland::text {

auto U16StringCharView::trimmed(const std::optional<CharSet> &characters, const std::optional<StringSide> side) const
    -> U16StringView {
    const auto trimSide = impl::U16StringTrimTools::sideFrom(side);
    if (characters.has_value()) {
        return withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    }
    return withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(trimSide));
}

}
