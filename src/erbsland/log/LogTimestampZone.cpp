// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogTimestampZone.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogTimestampZone::toString() const -> text::String {
    return _value == Local ? text::String{"local"_el} : text::String{"utc"_el};
}

auto LogTimestampZone::allStrings() -> text::StringList {
    return text::StringList{"utc"_el, "local"_el};
}

auto LogTimestampZone::fromString(const text::String &text) noexcept -> std::optional<LogTimestampZone> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("utc"_el, compare) == std::strong_ordering::equal) {
        return Utc;
    }
    if (text.compare("local"_el, compare) == std::strong_ordering::equal) {
        return Local;
    }
    return std::nullopt;
}

auto LogTimestampZone::fromStringOrThrow(const text::String &text) -> LogTimestampZone {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log timestamp zone."_el};
}

}
