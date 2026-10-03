// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogPath.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/AsciiCategory.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"
#include "../text/StringSide.hpp"
#include "../text/StringSplitter.hpp"

namespace erbsland::log {

using namespace text;
using namespace text::literals;

LogPath::LogPath(const String &value) : _value{value} {
    if (!isValid(value)) {
        throw err::ParameterFailure{"Invalid log path."_el, "value"_el};
    }
}

auto LogPath::contains(const LogPath &other) const noexcept -> bool {
    if (isRoot() || _value == other._value) {
        return true;
    }
    if (!other._value.startsWith(_value)) {
        return false;
    }
    return other._value.charAt(other._value.indexAt(StringSide::Front) + _value.length()) == U'/';
}

auto LogPath::isValid(const String &value) noexcept -> bool {
    if (value.isEmpty()) {
        return true;
    }
    if (value.length() > unit::ByteCount{255U}) {
        return false;
    }
    static const auto cAllowedCharacters = CharSet::fromPattern("-/0-9_a-z"_el);
    if (!value.containsOnly(cAllowedCharacters) || value.charAt(StringSide::Front) == U'/' ||
        value.charAt(StringSide::Back) == U'/') {
        return false;
    }
    auto splitter = StringSplitter{value, Char{U'/'}};
    auto segmentCount = std::size_t{};
    while (!splitter.isAtEnd()) {
        const auto segment = splitter.next();
        ++segmentCount;
        if (segmentCount > 16U || segment.isEmpty() ||
            !segment.charAt(StringSide::Front).isAsciiCategory(AsciiCategory::Alphanumeric) ||
            !segment.charAt(StringSide::Back).isAsciiCategory(AsciiCategory::Alphanumeric)) {
            return false;
        }
    }
    return true;
}

}
