// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogFileMode.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text;
using namespace text::literals;

auto LogFileMode::toString() const -> String {
    switch (_value) {
    case Overwrite:
        return "overwrite"_el;
    case Append:
        return "append"_el;
    }
    return {};
}

auto LogFileMode::allStrings() -> StringList {
    return StringList{"overwrite"_el, "append"_el};
}

auto LogFileMode::fromString(const String &text) noexcept -> std::optional<LogFileMode> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("overwrite"_el, compare) == std::strong_ordering::equal) {
        return Overwrite;
    }
    if (text.compare("append"_el, compare) == std::strong_ordering::equal) {
        return Append;
    }
    return std::nullopt;
}

auto LogFileMode::fromStringOrThrow(const String &text) -> LogFileMode {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log file mode."_el};
}

}
