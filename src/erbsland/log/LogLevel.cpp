// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLevel.hpp"

#include "../err/ParseError.hpp"
#include "../text/CaseSensitivity.hpp"
#include "../text/Literals.hpp"
#include "../text/StringList.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogLevel::toString() const -> text::String {
    switch (_value) {
    case Trace:
        return "trace"_el;
    case Information:
        return "information"_el;
    case Warning:
        return "warning"_el;
    case Error:
        return "error"_el;
    case All:
        return "all"_el;
    }
    return {};
}

auto LogLevel::toString(const LogLevelFormat format) const -> text::String {
    switch (format.toRawValue()) {
    case LogLevelFormat::ThreeLetterUpper:
        switch (_value) {
        case Trace:
            return "TRC"_el;
        case Information:
            return "INF"_el;
        case Warning:
            return "WRN"_el;
        case Error:
            return "ERR"_el;
        case All:
            break;
        }
        break;
    case LogLevelFormat::ShortLower:
        switch (_value) {
        case Trace:
            return "trc"_el;
        case Information:
            return "inf"_el;
        case Warning:
            return "wrn"_el;
        case Error:
            return "err"_el;
        case All:
            break;
        }
        break;
    case LogLevelFormat::FullLower:
        return toString();
    case LogLevelFormat::FullUpper:
        switch (_value) {
        case Trace:
            return "TRACE"_el;
        case Information:
            return "INFORMATION"_el;
        case Warning:
            return "WARNING"_el;
        case Error:
            return "ERROR"_el;
        case All:
            break;
        }
        break;
    }
    return "unknown"_el;
}

auto LogLevel::allStrings() -> text::StringList {
    return text::StringList{"trace"_el, "information"_el, "info"_el, "warning"_el, "warn"_el, "error"_el, "all"_el};
}

auto LogLevel::fromString(const text::String &text) noexcept -> std::optional<LogLevel> {
    const auto compare = ::erbsland::text::cCaseInsensitive.asciiComparisonFn();
    if (text.compare("trace"_el, compare) == std::strong_ordering::equal) {
        return Trace;
    }
    if (text.compare("information"_el, compare) == std::strong_ordering::equal ||
        text.compare("info"_el, compare) == std::strong_ordering::equal) {
        return Information;
    }
    if (text.compare("warning"_el, compare) == std::strong_ordering::equal ||
        text.compare("warn"_el, compare) == std::strong_ordering::equal) {
        return Warning;
    }
    if (text.compare("error"_el, compare) == std::strong_ordering::equal) {
        return Error;
    }
    if (text.compare("all"_el, compare) == std::strong_ordering::equal) {
        return All;
    }
    return std::nullopt;
}

auto LogLevel::fromStringOrThrow(const text::String &text) -> LogLevel {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log level."_el};
}

}
