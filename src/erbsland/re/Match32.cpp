// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Match32.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"
#include "../text/u32/U32String.hpp"

namespace erbsland::re {

using namespace text::literals;

auto Match32::content() const -> text::U32String {
    return content(0);
}

auto Match32::content(const CaptureGroupIndex groupIndex) const -> text::U32String {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterError{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return getContentForGroup(_groups[static_cast<std::size_t>(groupIndex)]);
}

auto Match32::content(const text::String &groupName) const -> text::U32String {
    return content(getGroupIndex(groupName));
}

}
