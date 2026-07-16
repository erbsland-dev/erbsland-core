// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Match.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::re {

using namespace text::literals;

auto Match::content() const -> text::StringView {
    return content(0);
}

auto Match::content(const CaptureGroupIndex groupIndex) const -> text::StringView {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterError{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return getContentForGroup(_groups[static_cast<std::size_t>(groupIndex)]);
}

auto Match::content(const text::StringView &groupName) const -> text::StringView {
    return content(getGroupIndex(groupName));
}

}
