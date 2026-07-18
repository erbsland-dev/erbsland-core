// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringEditor.hpp"

#include "impl/U16StringTrimTools.hpp"

namespace erbsland::text {

auto U16StringEditor::trim(const std::optional<CharSet> &characters, const std::optional<StringSide> side)
    -> U16StringEditor & {
    const auto trimSide = impl::U16StringTrimTools::sideFrom(side);
    if (characters.has_value()) {
        *this = withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    } else {
        *this = withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(trimSide));
    }
    return *this;
}

auto U16StringEditor::trimmed(const std::optional<CharSet> &characters, const std::optional<StringSide> side) const
    -> U16StringEditor {
    const auto trimSide = impl::U16StringTrimTools::sideFrom(side);
    if (characters.has_value()) {
        return withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(*characters, trimSide));
    }
    return withRange(impl::U16StringTrimTools{dataView()}.trimmedRange(trimSide));
}

}
