// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogTraceSection.hpp"

#include "../err/ParameterError.hpp"
#include "../text/AsciiCategory.hpp"
#include "../text/Literals.hpp"
#include "../text/StringSide.hpp"

namespace erbsland::log {

using namespace text::literals;

LogTraceSection::LogTraceSection(const text::String &value) : _value{value} {
    if (!isValid(value)) {
        throw err::ParameterError{"Invalid log trace section."_el, "value"_el};
    }
}

auto LogTraceSection::isValid(const text::String &value) noexcept -> bool {
    if (value.isEmpty()) {
        return true;
    }
    const auto first = value.charAt(text::StringSide::Front);
    return value.containsOnly(text::AsciiCategory::WordWithHyphen) &&
        (first == U'_' || first.isAsciiCategory(text::AsciiCategory::Letter));
}

}
