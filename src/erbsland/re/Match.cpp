// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Match.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"
#include "../text/String.hpp"

namespace erbsland::re {

using namespace text;
using namespace text::literals;

auto Match::content() const -> String {
    return content(0);
}

auto Match::content(const CaptureGroupIndex groupIndex) const -> String {
    if (!hasGroupIndex(groupIndex)) {
        throw err::ParameterFailure{"Group index is out of bounds."_el, "groupIndex"_el};
    }
    return getContentForGroup(_groups[static_cast<std::size_t>(groupIndex)]);
}

auto Match::content(const String &groupName) const -> String {
    return content(getGroupIndex(groupName));
}

}
