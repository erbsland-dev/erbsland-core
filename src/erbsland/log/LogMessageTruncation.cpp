// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogMessageTruncation.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogMessageTruncation::toString() const -> text::String {
    switch (_value) {
    case None:
        return "none"_el;
    case FirstLine:
        return "first_line"_el;
    case CharacterCount:
        return "characters"_el;
    case TotalLineLength:
        return "total_line"_el;
    }
    return {};
}

auto LogMessageTruncation::allStrings() -> text::StringList {
    return text::StringList{"none"_el, "first_line"_el, "characters"_el, "total_line"_el};
}

auto LogMessageTruncation::fromString(const text::String &text) noexcept -> std::optional<LogMessageTruncation> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("none"_el, compare) == std::strong_ordering::equal) {
        return None;
    }
    if (text.compare("first_line"_el, compare) == std::strong_ordering::equal) {
        return FirstLine;
    }
    if (text.compare("characters"_el, compare) == std::strong_ordering::equal) {
        return CharacterCount;
    }
    if (text.compare("total_line"_el, compare) == std::strong_ordering::equal) {
        return TotalLineLength;
    }
    return std::nullopt;
}

auto LogMessageTruncation::fromStringOrThrow(const text::String &text) -> LogMessageTruncation {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log message truncation."_el};
}

}
