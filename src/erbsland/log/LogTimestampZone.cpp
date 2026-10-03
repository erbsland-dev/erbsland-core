// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogTimestampZone.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text;
using namespace text::literals;

auto LogTimestampZone::toString() const -> String {
    return _value == Local ? String{"local"_el} : String{"utc"_el};
}

auto LogTimestampZone::allStrings() -> StringList {
    return StringList{"utc"_el, "local"_el};
}

auto LogTimestampZone::fromString(const String &text) noexcept -> std::optional<LogTimestampZone> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("utc"_el, compare) == std::strong_ordering::equal) {
        return Utc;
    }
    if (text.compare("local"_el, compare) == std::strong_ordering::equal) {
        return Local;
    }
    return std::nullopt;
}

auto LogTimestampZone::fromStringOrThrow(const String &text) -> LogTimestampZone {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log timestamp zone."_el};
}

}
