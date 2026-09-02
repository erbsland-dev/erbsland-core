// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLinePart.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::log {

using namespace text::literals;

auto LogLinePart::toString() const -> text::String {
    switch (_value) {
    case Time:
        return "time"_el;
    case Level:
        return "level"_el;
    case Name:
        return "name"_el;
    case Message:
        return "message"_el;
    case Literal:
    case _Count:
        return {};
    }
    return {};
}

auto LogLinePart::fromString(const text::String &text) noexcept -> std::optional<LogLinePart> {
    if (text == "time"_el) {
        return Time;
    }
    if (text == "level"_el) {
        return Level;
    }
    if (text == "name"_el) {
        return Name;
    }
    if (text == "message"_el) {
        return Message;
    }
    return std::nullopt;
}

auto LogLinePart::fromStringOrThrow(const text::String &text) -> LogLinePart {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"Unsupported log-line placeholder."_el};
}

}
