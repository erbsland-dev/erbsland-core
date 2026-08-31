// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogFileRotation.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogFileRotation::toString() const -> text::String {
    switch (_value) {
    case None:
        return "none"_el;
    case Hourly:
        return "hourly"_el;
    case Daily:
        return "daily"_el;
    case Weekly:
        return "weekly"_el;
    case Size:
        return "size"_el;
    }
    return {};
}

auto LogFileRotation::allStrings() -> text::StringList {
    return text::StringList{"none"_el, "hourly"_el, "daily"_el, "weekly"_el, "size"_el};
}

auto LogFileRotation::fromString(const text::String &text) noexcept -> std::optional<LogFileRotation> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("none"_el, compare) == std::strong_ordering::equal) {
        return None;
    }
    if (text.compare("hourly"_el, compare) == std::strong_ordering::equal) {
        return Hourly;
    }
    if (text.compare("daily"_el, compare) == std::strong_ordering::equal) {
        return Daily;
    }
    if (text.compare("weekly"_el, compare) == std::strong_ordering::equal) {
        return Weekly;
    }
    if (text.compare("size"_el, compare) == std::strong_ordering::equal) {
        return Size;
    }
    return std::nullopt;
}

auto LogFileRotation::fromStringOrThrow(const text::String &text) -> LogFileRotation {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log file rotation."_el};
}

}
