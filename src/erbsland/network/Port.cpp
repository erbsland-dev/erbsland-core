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
    auto options = IntegerParseOptions{};
    options.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpLength::one());
    const auto value = text.toInteger<uint32_t>(65536U, options);
    if (value > 65535U) {
        return std::nullopt;
    }
    return Port{static_cast<uint16_t>(value)};
}

auto Port::fromStringOrThrow(const String &text) -> Port {
    if (const auto result = fromString(text); result.has_value()) {
        return *result;
    }
    throw err::ParseError{"The text is not a valid network port."_el};
}

}
