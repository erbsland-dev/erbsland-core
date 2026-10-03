// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogTraceSection.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/AsciiCategory.hpp"
#include "../text/Char.hpp"
#include "../text/Literals.hpp"
#include "../text/StringSide.hpp"

namespace erbsland::log {

using namespace text;
using namespace text::literals;

LogTraceSection::LogTraceSection(const String &name) : _value{name} {
    if (!isValidName(name)) {
        throw err::ParameterFailure{"Invalid log trace section."_el, "value"_el};
    }
}

auto LogTraceSection::operator==(const LogTraceSection &other) const noexcept -> bool {
    return toString().compare(other.toString(), compareCharacters) == std::strong_ordering::equal;
}

auto LogTraceSection::compareCharacters(Char left, Char right) noexcept -> std::strong_ordering {
    if (left == U'-') {
        left = Char{U'_'};
    }
    if (right == U'-') {
        right = Char{U'_'};
    }
    return left.compareAsciiFolded(right);
}

auto LogTraceSection::isValidName(const String &name) noexcept -> bool {
    if (name.isEmpty()) {
        return true;
    }
    // Valid identifiers are ASCII, so their byte and character lengths are identical.
    if (name.length().toSizeT() > impl::cMaximumTraceSectionNameLength) {
        return false;
    }
    const auto first = name.charAt(StringSide::Front);
    return first.isAsciiLetter() && name.containsOnly(AsciiCategory::WordWithHyphen);
}

}
