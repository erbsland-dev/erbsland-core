// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Port.hpp"

#include "../err/ParseError.hpp"
#include "../text/IntegerBase.hpp"
#include "../text/IntegerParseOptions.hpp"
#include "../text/Literals.hpp"
#include "../unit/CpLength.hpp"

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto Port::toString() const -> String {
    return String::fromInteger(_value);
}

auto Port::fromString(const String &text) noexcept -> std::optional<Port> {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto Port::fromStringOrThrow(const String &text) -> Port {
    const auto options = IntegerParseOptions{}.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpLength::one());
    return Port{text.toIntegerOrThrow<uint16_t>(options)};
}

}
