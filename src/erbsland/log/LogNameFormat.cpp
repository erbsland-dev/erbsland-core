// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogNameFormat.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogNameFormat::toString() const -> text::String {
    switch (_value) {
    case Full:
        return "full"_el;
    case Leaf:
        return "leaf"_el;
    case HeadAndLeaf:
        return "head_and_leaf"_el;
    case LeftTruncated:
        return "left_truncated"_el;
    }
    return {};
}

auto LogNameFormat::allStrings() -> text::StringList {
    return text::StringList{"full"_el, "leaf"_el, "head_and_leaf"_el, "left_truncated"_el};
}

auto LogNameFormat::fromString(const text::String &text) noexcept -> std::optional<LogNameFormat> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("full"_el, compare) == std::strong_ordering::equal) {
        return Full;
    }
    if (text.compare("leaf"_el, compare) == std::strong_ordering::equal) {
        return Leaf;
    }
    if (text.compare("head_and_leaf"_el, compare) == std::strong_ordering::equal) {
        return HeadAndLeaf;
    }
    if (text.compare("left_truncated"_el, compare) == std::strong_ordering::equal) {
        return LeftTruncated;
    }
    return std::nullopt;
}

auto LogNameFormat::fromStringOrThrow(const text::String &text) -> LogNameFormat {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log name format."_el};
}

}
