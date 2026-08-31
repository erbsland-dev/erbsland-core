// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLevelFormat.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogLevelFormat::toString() const -> text::String {
    switch (_value) {
    case ThreeLetterUpper:
        return "short_upper"_el;
    case ShortLower:
        return "short_lower"_el;
    case FullLower:
        return "full_lower"_el;
    case FullUpper:
        return "full_upper"_el;
    }
    return {};
}

auto LogLevelFormat::allStrings() -> text::StringList {
    return text::StringList{"short_upper"_el, "short_lower"_el, "full_lower"_el, "full_upper"_el};
}

auto LogLevelFormat::fromString(const text::String &text) noexcept -> std::optional<LogLevelFormat> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("short_upper"_el, compare) == std::strong_ordering::equal) {
        return ThreeLetterUpper;
    }
    if (text.compare("short_lower"_el, compare) == std::strong_ordering::equal) {
        return ShortLower;
    }
    if (text.compare("full_lower"_el, compare) == std::strong_ordering::equal) {
        return FullLower;
    }
    if (text.compare("full_upper"_el, compare) == std::strong_ordering::equal) {
        return FullUpper;
    }
    return std::nullopt;
}

auto LogLevelFormat::fromStringOrThrow(const text::String &text) -> LogLevelFormat {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log level format."_el};
}

}
